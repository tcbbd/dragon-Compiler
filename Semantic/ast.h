#ifndef _AST_H_
#define _AST_H_

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <stdexcept>
#include "location.hh"

using namespace std;

class ASTNode {
public:
	enum NodeType {
		//Literal
		INTEGER,
		BOOLEAN,
		STRING,

		IDENTIFIER,
		TYPE,
		THIS,

		BINARY_EXPR,
		EXPR_LIST,
		FIELD_ACCESS,
		ARRAY_ACCESS,
		METHOD_INVOCATION,

		IF_THEN_ELSE_STMT,
		ELIF_LIST,

		WHILE_STMT,
		REPEAT_STMT,
		FOREACH_STMT,

		BREAK_STMT,
		CONTINUE_STMT,
		RETURN_STMT,
		PRINT_STMT,
		EXPR_STMT,
		EMPTY_STMT,
		BLOCK,

		PARAM_LIST,
		VARIABLE_DECL,
		VARIABLE_DECL_LIST,
		FUNC_DEFN,
		ARRAY_DECL,
		CLASS_DECL,
		CLASS_BODY,

		COMPOUND_DECL_LIST,
		PROGRAM,
	};

	ASTNode() {}
	virtual ~ASTNode() {
		for (int i = 0; i < children.size(); ++i)
			delete children[i];
	}

	vector<ASTNode *>& getChildren() { return children; }
	void setLoc(yy::location l) { loc = l; }
	yy::location getLoc() const { return loc; }

	virtual NodeType type() const = 0;
	virtual string print() = 0;
	virtual void traverse_draw_terminal(int i, string prefix = "") {
		cout << string(i * 4, ' ') << "|-" << prefix << print() << endl;
		for (int ii = 0; ii < children.size(); ++ii)
			children[ii]->traverse_draw_terminal(i + 1);
	}
	virtual void collect_info() {
		for (int i = 0; i < children.size(); ++i)
			children[i]->collect_info();
	};
protected:
	vector<ASTNode *> children;
	yy::location loc;
};

class ASTNodeList : public ASTNode {
public:
	void append(ASTNode *m) {
		if (m == NULL)
			throw runtime_error("ASTNodeList: append() called with Nullptr!\n");
		children.push_back(m);
	}
};

struct GenCodeInfo;

//---------------------------------------------------------------------

class ASTNodeExpression : public ASTNode {
public:
	virtual pair<bool, int> eval() = 0; // compute a constant expression
	virtual string gen_code(GenCodeInfo* gen_code_info);
};

class ASTNodeExpressionList : public ASTNodeList {
public:
	NodeType type() const { return EXPR_LIST; }
	string print() { return string("expr_list") + (children.empty() ? ": No arg!" : ""); }
};

class ASTNodeBinaryExpr : public ASTNodeExpression {
public:
	ASTNodeBinaryExpr(ASTNodeExpression *expr1, ASTNodeExpression *expr2,
			string o) : op(o) {
		if (expr1 == NULL || expr2 == NULL)
			throw runtime_error("ASTNodeBinaryExpr: Constructor called with Nullptr!\n");
		children.push_back(expr1);
		children.push_back(expr2);
	}
	~ASTNodeBinaryExpr() {}

	NodeType type() const { return BINARY_EXPR; }
	string print() { return "expr: " + op; }
	pair<bool, int> eval();

	string gen_code(GenCodeInfo* gen_code_info);
private:
	string gen_assign(GenCodeInfo *gen_code_info);
	string gen_compute(GenCodeInfo *gen_code_info);
	string gen_compute_load(GenCodeInfo *gen_code_info, bool &isconstant, bool &isbool, int &value, bool isleft);
	string op;
};

class ASTNodePrimary : public ASTNodeExpression {
public:
	enum LvalType {
		ID,
		THISPOINTER,
		COMPOSED
	};
};

class ASTNodeLiteral : public ASTNodePrimary {
};

//------------------------------------------------------------------

class ASTNodeID : public ASTNodePrimary {
public:
	ASTNodeID(string s) : id(s) {}
	~ASTNodeID() {}

	NodeType type() const { return IDENTIFIER; }
	string print() { return "ID: " + id; }
	string getID() { return id; }
	pair<bool, int> eval() { return make_pair(false, 0); }
private:
	string id;
};

class ASTNodeThis : public ASTNodePrimary {
public:
	NodeType type() const { return THIS; }
	string print() { return "this"; }
	pair<bool, int> eval() { return make_pair(false, 0); }
};

class ASTNodeType : public ASTNode {
public:
	enum VariableType{
		INTEGER,
		BOOLEAN,
		ARRAY,
		CLASS,
		UNKNOWN,
		// void return value OR no super class
		VOID
	};

	ASTNodeType(string type) : value(type) {
		if (type == "integer") {
			variable_type = INTEGER;
			asm_str = "i32";
		}
		else if (type == "boolean") {
			variable_type = BOOLEAN;
			asm_str = "i8";
		}
		else
			variable_type = UNKNOWN;
	}
	ASTNodeType(VariableType type) : variable_type(type) {
		if (type == INTEGER) {
			value = "integer";
			asm_str = "i32";
		}
		else if (type == BOOLEAN) {
			value = "boolean";
			asm_str = "i8";
		}
	}
	~ASTNodeType() {}

	NodeType type() const { return TYPE; }
	VariableType variableType() { return variable_type; }
	void setVariableType(VariableType type) {
		variable_type = type;
		if (type == INTEGER) {
			value = "integer";
			asm_str = "i32";
		}
		else if (type == BOOLEAN) {
			value = "boolean";
			asm_str = "i8";
		}
	}
	string print() { return "type: " + value; }
	string getValue() { return value; }
	void setValue(string type) {
		value = type;
		if (type == "integer") {
			variable_type = INTEGER;
			asm_str = "i32";
		}
		else if (type == "boolean") {
			variable_type = BOOLEAN;
			asm_str = "i8";
		}
		else
			variable_type = UNKNOWN;
	}
	void setAsm(string as) { asm_str = as; }
	string getAsm() { return asm_str; }
	string getTypeAsm(bool array_ref);
private:
	VariableType variable_type;
	string value;
	string asm_str;
};

//-----------------------------primary Begin-------------------------------------

class ASTNodeFieldAccess : public ASTNodePrimary {
public:
	ASTNodeFieldAccess(ASTNodePrimary *pri, ASTNodeID *id) {
		if (pri == NULL || id == NULL)
			throw runtime_error("ASTNodeFieldAccess: Constructor called with Nullptr!\n");
		children.push_back(pri);
		children.push_back(id);
	}
	~ASTNodeFieldAccess() {}

	NodeType type() const { return FIELD_ACCESS; }
	string print() { return "field access"; }
	pair<bool, int> eval() { return make_pair(false, 0); }
	string gen_code(GenCodeInfo* gen_code_info);
private:
	string gen_simple(GenCodeInfo *gen_code_info);
	string gen_composed(GenCodeInfo* gen_code_info);
	string gen_asm(GenCodeInfo* gen_code_info, LvalType lvaltype);
};

class ASTNodeArrayAccess : public ASTNodePrimary {
public:
	ASTNodeArrayAccess(ASTNodePrimary *pri, ASTNodeExpression *expr) {
		if (pri == NULL || expr == NULL)
			throw runtime_error("ASTNodeArrayAccess: Constructor called with Nullptr!\n");
		children.push_back(pri);
		children.push_back(expr);
	}
	~ASTNodeArrayAccess() {}

	NodeType type() const { return ARRAY_ACCESS; }
	string print() { return "array access"; }
	pair<bool, int> eval() { return make_pair(false, 0); }

	void traverse_draw_terminal(int i, string prefix = "") {
		cout << string(i * 4, ' ') << "|-" << prefix << print() << endl;
		children[0]->traverse_draw_terminal(i + 1, "array: ");
		children[1]->traverse_draw_terminal(i + 1, "index: ");
	}
	string gen_code(GenCodeInfo* gen_code_info);
private:
	void check_type(GenCodeInfo* gen_code_info, ASTNodeType* type);
	string gen_simple(GenCodeInfo *gen_code_info);
	string gen_asm(GenCodeInfo* gen_code_info, LvalType lvaltype);
};

class ASTNodeMethodInvocation : public ASTNodePrimary {
public:
	ASTNodeMethodInvocation(ASTNodeID *id, ASTNodeExpressionList *expr_list) {
		if (id == NULL || expr_list == NULL)
			throw runtime_error("ASTNodeMethodInvocation: Constructor called with Nullptr!\n");
		children.push_back(id);
		children.push_back(expr_list);
	}
	ASTNodeMethodInvocation(ASTNodeFieldAccess *facc, ASTNodeExpressionList *expr_list) {
		if (facc == NULL || expr_list == NULL)
			throw runtime_error("ASTNodeMethodInvocation: Constructor called with Nullptr!\n");
		children.push_back(facc);
		children.push_back(expr_list);
	}
	~ASTNodeMethodInvocation() {}

	NodeType type() const { return METHOD_INVOCATION; }
	string print() { return "method invocation"; }
	pair<bool, int> eval() { return make_pair(false, 0); }

	void traverse_draw_terminal(int i, string prefix = "") {
		cout << string(i * 4, ' ') << "|-" << prefix << print() << endl;
		children[0]->traverse_draw_terminal(i + 1, "method: ");
		children[1]->traverse_draw_terminal(i + 1, "args: ");
	}
	string gen_code(GenCodeInfo* gen_code_info);
private:
};

//-----------------------------primary End------------------------------------
//----------------------------literal Begin-----------------------------------

class ASTNodeInteger : public ASTNodeLiteral {
public:
	ASTNodeInteger(int i) : value(i) {}
	~ASTNodeInteger() {}

	NodeType type() const { return INTEGER; }
	int getValue() { return value; }
	string print() {
		stringstream ss;
		ss << value;
		return "INT: " + ss.str();
	}
	pair<bool, int> eval() { return make_pair(true, value); }
private:
	int value;
};

class ASTNodeBoolean : public ASTNodeLiteral {
public:
	ASTNodeBoolean(bool i) : value(i) {}
	~ASTNodeBoolean() {}

	NodeType type() const { return BOOLEAN; }
	bool getValue() { return value; }
	string print() {
		if (value)
			return "BOOL: true";
		else
			return "BOOL: false";
	}
	pair<bool, int> eval() { return make_pair(true, value); }
private:
	bool value;
};

class ASTNodeString : public ASTNodeLiteral {
public:
	ASTNodeString(string s) : value(s) {}
	~ASTNodeString() {}

	NodeType type() const { return STRING; }
	string getValue() { return value; }
	string print() { return "STRING: " + value; }
	pair<bool, int> eval() { return make_pair(false, 0); }
	void collect_info();
private:
	string value;
};

//----------------------------literal End-------------------------------------
//----------------------------statement Begin---------------------------------

class ASTNodeStatement : public ASTNode {
public:
	virtual string gen_code(GenCodeInfo* gen_code_info) = 0;
};

class ASTNodeBlock : public ASTNodeList {
public:
	NodeType type() const { return BLOCK; }
	string print() { return string("block") + (children.empty() ? ": Empty block!" : ""); }
	string gen_code(GenCodeInfo* gen_code_info);
};

//------------------Selection Statement-------------------------

class ASTNodeElifList : public ASTNodeList {
public:
	ASTNodeElifList(ASTNodeExpression *expr, ASTNodeBlock *blk) { append(expr, blk); }
	~ASTNodeElifList() {}

	void append(ASTNodeExpression *expr, ASTNodeBlock *blk) {
		if (expr == NULL || blk == NULL)
			throw runtime_error("ASTNodeElifList: append() called with Nullptr!\n");
		children.push_back(expr);
		children.push_back(blk);
	}
	NodeType type() const { return ELIF_LIST; }
	string print() { return "elif list"; }
};

class ASTNodeIfThenElseStmt : public ASTNodeStatement {
public:
	ASTNodeIfThenElseStmt(ASTNodeExpression *expr, ASTNodeBlock *blk1, ASTNodeBlock *blk2) {
		if (expr == NULL || blk1 == NULL)
			throw runtime_error("ASTNodeIfThenElseStmt: Constructor called with Nullptr!\n");
		children.push_back(expr);
		children.push_back(blk1);
		if (blk2)
			children.push_back(blk2);
	}
	ASTNodeIfThenElseStmt(ASTNodeExpression *expr, ASTNodeBlock *blk1, ASTNodeElifList *elif, ASTNodeBlock *blk2) {
		if (expr == NULL || blk1 == NULL || elif == NULL || blk2 == NULL)
			throw runtime_error("ASTNodeIfThenElseStmt: Constructor called with Nullptr!\n");
		children.push_back(expr);
		children.push_back(blk1);
		vector<ASTNode *>& elif_children = elif->getChildren();
		for (int i = 0; i < elif_children.size(); ++i)
			children.push_back(elif_children[i]);
		children.push_back(blk2);
	}
	~ASTNodeIfThenElseStmt() {}

	NodeType type() const { return IF_THEN_ELSE_STMT; }
	string print() {
		if (children.size() == 2)
			return "if then statement";
		else
			return "if then else statement";
	}

	void traverse_draw_terminal(int i, string prefix = "") {
		cout << string(i * 4, ' ') << "|-" << prefix << print() << endl;
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: if" << endl;
		children[0]->traverse_draw_terminal(i + 1);
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: then" << endl;
		children[1]->traverse_draw_terminal(i + 1);
		if (children.size() == 2) {
			cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: end if" << endl;
			return;
		}

		int ii = 2;
		for (; ii < children.size() - 1; ii += 2) {
			cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: elif" << endl;
			children[ii]->traverse_draw_terminal(i + 1);
			cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: then" << endl;
			children[ii + 1]->traverse_draw_terminal(i + 1);
		}
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: else" << endl;
		children[ii]->traverse_draw_terminal(i + 1);
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: end if" << endl;
	}
	string gen_code(GenCodeInfo* gen_code_info);
};

//------------------Iteration Statement-------------------------

class ASTNodeWhileStmt : public ASTNodeStatement {
public:
	ASTNodeWhileStmt(ASTNodeExpression *expr, ASTNodeBlock *blk) {
		if (expr == NULL || blk == NULL)
			throw runtime_error("ASTNodeWhileStmt: Constructor called with Nullptr!\n");
		children.push_back(expr);
		children.push_back(blk);
	}
	~ASTNodeWhileStmt() {}

	NodeType type() const { return WHILE_STMT; }
	string print() { return "while statement"; }

	void traverse_draw_terminal(int i, string prefix = "") {
		cout << string(i * 4, ' ') << "|-" << prefix << print() << endl;
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: while" << endl;
		children[0]->traverse_draw_terminal(i + 1);
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: do" << endl;
		children[1]->traverse_draw_terminal(i + 1);
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: end while" << endl;
	}
	string gen_code(GenCodeInfo* gen_code_info);
};

class ASTNodeRepeatStmt : public ASTNodeStatement {
public:
	ASTNodeRepeatStmt(ASTNodeBlock *blk, ASTNodeExpression *expr) {
		if (blk == NULL || expr == NULL)
			throw runtime_error("ASTNodeRepeatStmt: Constructor called with Nullptr!\n");
		children.push_back(blk);
		children.push_back(expr);
	}
	~ASTNodeRepeatStmt() {}

	NodeType type() const { return REPEAT_STMT; }
	string print() { return "repeat statement"; }

	void traverse_draw_terminal(int i, string prefix = "") {
		cout << string(i * 4, ' ') << "|-" << prefix << print() << endl;
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: repeat" << endl;
		children[0]->traverse_draw_terminal(i + 1);
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: until" << endl;
		children[1]->traverse_draw_terminal(i + 1);
	}
	string gen_code(GenCodeInfo* gen_code_info);
};

class ASTNodeForEachStmt : public ASTNodeStatement {
public:
	ASTNodeForEachStmt(ASTNodeID *id, ASTNodeExpression *expr, ASTNodeBlock *blk) {
		if (id == NULL || expr == NULL || blk == NULL)
			throw runtime_error("ASTNodeForEachStmt: Constructor called with Nullptr!\n");
		children.push_back(id);
		children.push_back(expr);
		children.push_back(blk);
	}
	~ASTNodeForEachStmt() {}

	NodeType type() const { return FOREACH_STMT; }
	string print() { return "foreach statement"; }

	void traverse_draw_terminal(int i, string prefix = "") {
		cout << string(i * 4, ' ') << "|-" << prefix << print() << endl;
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: foreach" << endl;
		children[0]->traverse_draw_terminal(i + 1);
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: in" << endl;
		children[1]->traverse_draw_terminal(i + 1);
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: do" << endl;
		children[2]->traverse_draw_terminal(i + 1);
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: end foreach" << endl;
	}
	string gen_code(GenCodeInfo* gen_code_info);
};

//-------------------------Others-------------------------------

class ASTNodeBreakStmt : public ASTNodeStatement {
public:
	NodeType type() const { return BREAK_STMT; }
	string print() { return "break statement"; }
	string gen_code(GenCodeInfo* gen_code_info);
};

class ASTNodeContinueStmt : public ASTNodeStatement {
public:
	NodeType type() const { return CONTINUE_STMT; }
	string print() { return "continue statement"; }
	string gen_code(GenCodeInfo* gen_code_info);
};

class ASTNodeReturnStmt : public ASTNodeStatement {
public:
	ASTNodeReturnStmt() {}
	ASTNodeReturnStmt(ASTNodeExpression *expr) { children.push_back(expr); }
	~ASTNodeReturnStmt() {}

	NodeType type() const { return RETURN_STMT; }
	string print() { return "return statement"; }
	string gen_code(GenCodeInfo* gen_code_info);
};

class ASTNodePrintStmt : public ASTNodeStatement {
public:
	ASTNodePrintStmt(ASTNodeExpressionList *expr_list) {
		if (expr_list == NULL)
			throw runtime_error("ASTNodePrintStmt: Constructor called with Nullptr!\n");
		children.push_back(expr_list);
	}
	~ASTNodePrintStmt() {}

	NodeType type() const { return PRINT_STMT; }
	string print() { return "print statement"; }

	void traverse_draw_terminal(int i, string prefix = "") {
		cout << string(i * 4, ' ') << "|-" << prefix << print() << endl;
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: print" << endl;
		children[0]->traverse_draw_terminal(i + 1);
	}
	string gen_code(GenCodeInfo* gen_code_info);
private:
	string gen_code(GenCodeInfo* gen_code_info, ASTNodeExpression* expr);
	string gen_simple(GenCodeInfo* gen_code_info);
	string gen_composed(GenCodeInfo* gen_code_info);
};

class ASTNodeExpressionStmt : public ASTNodeStatement {
public:
	ASTNodeExpressionStmt() {}
	ASTNodeExpressionStmt(ASTNodeExpression *expr) { children.push_back(expr); }
	~ASTNodeExpressionStmt() {}

	NodeType type() const {
		if (children.empty())
			return EMPTY_STMT;
		else
			return EXPR_STMT;
	}
	string print() {
		if (children.empty())
			return "empty statement";
		else
			return "expression statement";
	}
	string gen_code(GenCodeInfo* gen_code_info) {
		if (children.empty())
			return "";
		else
			return dynamic_cast<ASTNodeExpression*>(children[0])->gen_code(gen_code_info);
	}
};

//----------------------------statement End------------------------------------
//------------------------------declaration Begin------------------------------

class ASTNodeDeclaration : public ASTNode {
};

class ASTNodeDeclList : public ASTNodeList {
};

class ASTNodeParameterList : public ASTNodeList {
public:
	NodeType type() const { return PARAM_LIST; }
	string print() { return string("parameter list") + (children.empty() ? ": No param!" : ""); }
};

class ASTNodeVariableDecl : public ASTNodeDeclaration {
public:
	ASTNodeVariableDecl (ASTNodeID *id, ASTNodeType *type) {
		if (id == NULL || type == NULL)
			throw runtime_error("ASTNodeVariableDecl: Constructor called with Nullptr!\n");
		children.push_back(id);
		children.push_back(type);
	}
	~ASTNodeVariableDecl() {}

	NodeType type() const { return VARIABLE_DECL; }
	string print() { return "variable declaration";}

	void traverse_draw_terminal(int i, string prefix = "") {
		cout << string(i * 4, ' ') << "|-" << prefix << print() << endl;
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: var" << endl;
		children[0]->traverse_draw_terminal(i + 1);
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: is" << endl;
		children[1]->traverse_draw_terminal(i + 1);
	}
};

class ASTNodeVariableDeclList : public ASTNodeDeclList {
public:
	NodeType type() const { return VARIABLE_DECL_LIST; }
	string print() { return string("variable declaration list") +
		(children.empty() ? ": No decl!" : ""); }
};

class ASTNodeClassDecl;
class ASTNodeClassBody;

class ASTNodeFunctionDefn : public ASTNodeDeclaration {
public:
	ASTNodeFunctionDefn(ASTNodeID *id, ASTNodeParameterList *param_list,
			ASTNodeVariableDeclList *param_decl, ASTNodeType *type,
			ASTNodeVariableDeclList *local_decl, ASTNodeBlock *blk) {
		if (id == NULL || param_list == NULL || param_decl == NULL || type ==
				NULL || local_decl == NULL || blk == NULL)
			throw runtime_error("ASTNodeFunctionDecl: Constructor called with Nullptr!\n");
		children.push_back(id);
		children.push_back(param_list);
		children.push_back(param_decl);
		children.push_back(type);
		children.push_back(local_decl);
		children.push_back(blk);
	}
	~ASTNodeFunctionDefn() {}

	NodeType type() const { return FUNC_DEFN; }
	string print() { return "function definition"; }

	void traverse_draw_terminal(int i, string prefix = "") {
		cout << string(i * 4, ' ') << "|-" << prefix << print() << endl;
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: function" << endl;
		children[0]->traverse_draw_terminal(i + 1, "function name: ");
		children[1]->traverse_draw_terminal(i + 1, "function parameters: ");
		children[2]->traverse_draw_terminal(i + 1, "function parameter declarations: ");
		if (dynamic_cast<ASTNodeType *>(children[3])->variableType() != ASTNodeType::VOID) {
			cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: return" << endl;
			children[3]->traverse_draw_terminal(i + 1);
		}
		else
			cout << string(i * 4 + 4, ' ') << "|-" << "NO return value!" << endl;
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: is" << endl;
		children[4]->traverse_draw_terminal(i + 1, "function local variable declarations: ");
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: begin" << endl;
		children[5]->traverse_draw_terminal(i + 1, "function body: ");
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: end function" << endl;
	}
	map<string, pair<int, ASTNodeType*>>* getParams() { return &params; }
	void collect_info();
	void collect_info(map<string, ASTNodeFunctionDefn*> &func_table);
	void gen_code(string class_id = "", ASTNodeClassBody *class_body = NULL);
private:
	map<string, pair<int, ASTNodeType*>> params;
	map<string, ASTNodeType*> localvar_table;
};

class ASTNodeArrayDecl : public ASTNodeDeclaration {
public:
	ASTNodeArrayDecl(ASTNodeID *id, ASTNodeExpression *expr, ASTNodeType *type)
	: length(0), indegree(0) {
		if (id == NULL || expr == NULL || type == NULL)
			throw runtime_error("ASTNodeFunctionDecl: Constructor called with Nullptr!\n");
		children.push_back(id);
		children.push_back(expr);
		children.push_back(type);
	}
	~ASTNodeArrayDecl() {}

	NodeType type() const { return ARRAY_DECL; }
	string print() { return "array declaration: "; }

	void traverse_draw_terminal(int i, string prefix = "") {
		cout << string(i * 4, ' ') << "|-" << prefix << print() << endl;
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: type" << endl;
		children[0]->traverse_draw_terminal(i + 1);
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: is array of" << endl;
		children[1]->traverse_draw_terminal(i + 1, "array length: ");
		children[2]->traverse_draw_terminal(i + 1, "array type: ");
	}
	void collect_info();

	int getLength() { return length; }
	void depend(ASTNodeArrayDecl* vertex) {
		indegree++;
		vertex->depender.push_back(this);
	}
	int getIndegree() { return indegree; }
	void decreaseIndegree() { indegree--; }
	vector<ASTNodeArrayDecl*>& getDepender() { return depender; }
private:
	int length;

	// to construct array dependency graph
	vector<ASTNodeArrayDecl*> depender;
	int indegree;
};

class ASTNodeClassBody : public ASTNodeDeclList {
public:
	ASTNodeClassBody() : indegree(0), visited(false) {}
	~ASTNodeClassBody() {}

	NodeType type() const { return CLASS_BODY; }
	string print() { return string("class body: ") + (children.empty() ? ": Empty class body!" : ""); }
	void collect_info();
	void merge(ASTNodeClassBody* super);
	map<string, ASTNodeFunctionDefn*>* getFuncTable() { return &func_table; }
	map<string, pair<int, ASTNodeType*>>* getVarTable() { return &var_table; }

	void depend(string info, ASTNodeClassBody* vertex) {
		indegree++;
		vertex->depender.push_back(make_pair(info, this));
	}
	int getIndegree() { return indegree; }
	void decreaseIndegree() { indegree--; }
	vector<pair<string, ASTNodeClassBody*>>& getDepender() { return depender; }
	void setAsm(string as) { asm_str = as; }
	string getAsm() { return asm_str; }

	ASTNodeClassBody* dfs() {
		visited = true;
		if (depender.empty())
			return NULL;
		for (int i = 0; i < depender.size(); ++i) {
			visited_var = i;
			if (depender[i].second->visited)
				return depender[i].second;
			else {
				ASTNodeClassBody* ret = depender[i].second->dfs();
				if (ret == NULL)
					continue;
				else
					return ret;
			}
		}
		return NULL;
	}
	void printinfo(stringstream& ss) {
		visited = false;
		if (depender[visited_var].second->visited)
			depender[visited_var].second->printinfo(ss);
		ss << depender[visited_var].first;
	}

	void gen_code(string id) {
		for (auto i : func_table)
			i.second->gen_code(id, this);
	}
private:
	map<string, ASTNodeFunctionDefn*> func_table;
	map<string, pair<int, ASTNodeType*>> var_table;

	// to construct class dependency graph (ASTNodeType* is type of class itself, not super class)
	vector<pair<string, ASTNodeClassBody*>> depender;
	int indegree;
	string asm_str;
	bool visited;
	int visited_var;
};

class ASTNodeClassDecl : public ASTNodeDeclaration {
public:
	ASTNodeClassDecl(ASTNodeID *id, ASTNodeType *super, ASTNodeClassBody *body) {
		if (id == NULL || super == NULL || body == NULL)
			throw runtime_error("ASTNodeClassDecl: Constructor called with Nullptr!\n");
		children.push_back(id);
		children.push_back(super);
		children.push_back(body);
	}
	~ASTNodeClassDecl() {}

	NodeType type() const { return CLASS_DECL; }
	string print() { return "class declaration: "; }

	void traverse_draw_terminal(int i, string prefix = "") {
		cout << string(i * 4, ' ') << "|-" << prefix << print() << endl;
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: type" << endl;
		children[0]->traverse_draw_terminal(i + 1, "class name: ");
		if (dynamic_cast<ASTNodeType *>(children[1])->variableType() != ASTNodeType::VOID) {
			cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: extends" << endl;
			children[1]->traverse_draw_terminal(i + 1);
		}
		else
			cout << string(i * 4 + 4, ' ') << "|-" << "NO super class!" << endl;
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: is class" << endl;
		children[2]->traverse_draw_terminal(i + 1);
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: end class" << endl;
	}
	void collect_info();
};

class ASTNodeCompoundDeclList : public ASTNodeDeclList {
public:
	NodeType type() const { return COMPOUND_DECL_LIST; }
	string print() { return string("global declarations: ") + (children.empty() ? ": No decl!": ""); }
};

//------------------------------declaration End--------------------------------

class ASTNodeProgram : public ASTNode {
public:
	ASTNodeProgram(ASTNodeID *id, ASTNodeCompoundDeclList *cmpd_decl,
			ASTNodeVariableDeclList *local_decl, ASTNodeBlock *blk) {
		if (id == NULL || cmpd_decl == NULL || local_decl == NULL || blk == NULL)
			throw runtime_error("ASTNodeProgram: Constructor called with Nullptr!\n");
		children.push_back(id);
		children.push_back(cmpd_decl);
		children.push_back(local_decl);
		children.push_back(blk);
	}
	~ASTNodeProgram() {}

	NodeType type() const { return PROGRAM; }
	string print() { return "program: "; }

	void traverse_draw_terminal(int i, string prefix = "") {
		cout << string(i * 4, ' ') << "|-" << prefix << print() << endl;
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: program" << endl;
		children[0]->traverse_draw_terminal(i + 1, "program name: ");
		children[1]->traverse_draw_terminal(i + 1);
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: is" << endl;
		children[2]->traverse_draw_terminal(i + 1);
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: begin" << endl;
		children[3]->traverse_draw_terminal(i + 1, "program body: ");
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: end" << endl;
	}
	void collect_info();
	void gen_code();
	void gen_typedef();
private:
	map<string, ASTNodeType*> localvar_table;
};

#endif // _AST_H_
