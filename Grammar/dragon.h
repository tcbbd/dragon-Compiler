#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
using namespace std;

class ASTNode {
public:
	enum NodeType {
		//Literal
		INTEGER,
		BOOLEAN,
		STRING,

		IDENTIFIER,
		NAME,
		TYPE,

		BINARY_EXPR,
		// TODO add unary expressions
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
		FUNC_DECL,
		FUNC_DEFN,
		ARRAY_DECL,
		CLASS_DECL,
		CLASS_BODY,

		COMPOUND_DECL_LIST,
		PROGRAM,
	};

	ASTNode() : child(NULL), sibling(NULL) {}
	virtual ~ASTNode() {
		while (child) {
			ASTNode *tmp = child->sibling;
			delete child;
			child = tmp;
		}
	}

	virtual NodeType type() = 0;
	virtual string print() = 0;
	
	ASTNode* getChild() {
		return child;
	}
	void setChild(ASTNode *child) {
		this->child = child;
	}
	ASTNode* getSibling() {
		return sibling;
	}
	void setSibling(ASTNode *sibling) {
		this->sibling = sibling;
	}

	virtual void traverse_draw_terminal(int i, string prefix = "") {
		cout << string(i * 4, ' ') << "|-" << prefix << print() << endl;
		ASTNode *tmp1 = child;
		while (tmp1) {
			ASTNode *tmp2 = tmp1->sibling;
			tmp1->traverse_draw_terminal(i + 1);
			tmp1 = tmp2;
		}
	}
protected:
	void addchild(ASTNode *c) {
		if (child) {
			ASTNode *tmp = child;
			child = c;
			c->sibling = tmp;
		}
		else
			child = c;
	}

	ASTNode *child;
	ASTNode *sibling;
};

class ASTNodeList : public ASTNode {
protected:
	void ASTNodeListConstruct(ASTNode *member) {
		child = member;
	}
	void ASTNodeListConstruct(ASTNodeList *lst, ASTNode *member) {
		child = lst->child;
		sibling = lst->sibling;
		ASTNode *tmp1 = child;
		ASTNode *tmp2 = child->getSibling();
		while (tmp2) {
			tmp1 = tmp2;
			tmp2 = tmp2->getSibling();
		}
		tmp1->setSibling(member);
	}
};

//---------------------------------------------------------------------

class ASTNodeExpression : public ASTNode {
};

class ASTNodeExpressionList : public ASTNodeList {
public:
	ASTNodeExpressionList(ASTNodeExpression *expr) {
		ASTNodeListConstruct(expr);
	}
	ASTNodeExpressionList(ASTNodeExpressionList *expr_list, ASTNodeExpression *expr) {
		if (expr_list == NULL || expr == NULL)
			throw runtime_error("ASTNodeExpressionList: Constructor called with Nullptr!\n");
		ASTNodeListConstruct(expr_list, expr);
	}
	~ASTNodeExpressionList() {}

	NodeType type() { return EXPR_LIST; }
	string print() { return string("expr_list") + (child ? "" : ": No arg!"); }
};

class ASTNodeBinaryExpr : public ASTNodeExpression {
public:
	ASTNodeBinaryExpr(ASTNodeExpression *expr1, ASTNodeExpression *expr2,
			string o) : op(o) {
		if (expr1 == NULL || expr2 == NULL)
			throw runtime_error("ASTNodeBinaryExpr: Constructor called with Nullptr!\n");
		child = expr2;
		addchild(expr1);
	}
	~ASTNodeBinaryExpr() {}

	NodeType type() { return BINARY_EXPR; }
	string print() { return "expr: " + op; }
private:
	string op;
};

class ASTNodePrimary : public ASTNodeExpression {
};

class ASTNodeLiteral : public ASTNodePrimary {
};

//------------------------------------------------------------------

class ASTNodeID : public ASTNodePrimary {
public:
	ASTNodeID(string s) : id(s) {}
	~ASTNodeID() {}

	NodeType type() { return IDENTIFIER; }
	string print() { return "ID: " + id; }
	string getID() { return id; }
private:
	string id;
};

class ASTNodeType : public ASTNode {
public:
	enum VariableType{
		INTEGER,
		BOOLEAN,
		ARRAY,
		CLASS,
		UNKNOWN,
		// can only be used in functions which return void
		VOID
	};

	ASTNodeType(string type) : value(type) {
		if (type == "integer")
			variable_type = INTEGER;
		else if (type == "boolean")
			variable_type = BOOLEAN;
		else
			variable_type = UNKNOWN;
	}
	ASTNodeType(VariableType type) : variable_type(type) {
		if (type == INTEGER)
			value = "integer";
		else if (type == BOOLEAN)
			value = "boolean";
	}
	~ASTNodeType() {}

	NodeType type() { return TYPE; }
	VariableType variableType() { return variable_type; }
	string print() { return "type: " + value; }
private:
	VariableType variable_type;
	string value;
};

//-----------------------------primary Begin-------------------------------------

class ASTNodeFieldAccess : public ASTNodePrimary {
public:
	ASTNodeFieldAccess(ASTNodePrimary *pri, ASTNodeID *id) {
		if (pri == NULL || id == NULL)
			throw runtime_error("ASTNodeFieldAccess: Constructor called with Nullptr!\n");
		child = id;
		addchild(pri);
	}
	~ASTNodeFieldAccess() {}

	NodeType type() { return FIELD_ACCESS; }
	string print() { return "field access"; }
};

class ASTNodeArrayAccess : public ASTNodePrimary {
public:
	ASTNodeArrayAccess(ASTNodePrimary *pri, ASTNodeExpression *expr) {
		if (pri == NULL || expr == NULL)
			throw runtime_error("ASTNodeArrayAccess: Constructor called with Nullptr!\n");
		child = expr;
		addchild(pri);
	}
	~ASTNodeArrayAccess() {}

	NodeType type() { return ARRAY_ACCESS; }
	string print() { return "array access"; }

	void traverse_draw_terminal(int i, string prefix = "") {
		cout << string(i * 4, ' ') << "|-" << prefix << print() << endl;
		child->traverse_draw_terminal(i + 1, "array: ");
		child->getSibling()->traverse_draw_terminal(i + 1, "index: ");
	}
};

class ASTNodeMethodInvocation : public ASTNodePrimary {
public:
	ASTNodeMethodInvocation(ASTNodeID *id, ASTNodeExpressionList *expr_list) {
		if (id == NULL || expr_list == NULL)
			throw runtime_error("ASTNodeMethodInvocation: Constructor called with Nullptr!\n");
		child = expr_list;
		addchild(id);
	}
	ASTNodeMethodInvocation(ASTNodeFieldAccess *facc, ASTNodeExpressionList *expr_list) {
		if (facc == NULL || expr_list == NULL)
			throw runtime_error("ASTNodeMethodInvocation: Constructor called with Nullptr!\n");
		child = expr_list;
		addchild(facc);
	}
	~ASTNodeMethodInvocation() {}

	NodeType type() { return METHOD_INVOCATION; }
	string print() { return "method invocation"; }

	void traverse_draw_terminal(int i, string prefix = "") {
		cout << string(i * 4, ' ') << "|-" << prefix << print() << endl;
		child->traverse_draw_terminal(i + 1, "method: ");
		child->getSibling()->traverse_draw_terminal(i + 1, "args: ");
	}
};

//-----------------------------primary End------------------------------------
//----------------------------literal Begin-----------------------------------

class ASTNodeInteger : public ASTNodeLiteral {
public:
	ASTNodeInteger(int i) : value(i) {}
	~ASTNodeInteger() {}

	NodeType type() { return INTEGER; }
	string print() {
		stringstream ss;
		ss << value;
		return "INT: " + ss.str();
	}
private:
	int value;
};

class ASTNodeBoolean : public ASTNodeLiteral {
public:
	ASTNodeBoolean(bool i) : value(i) {}
	~ASTNodeBoolean() {}

	NodeType type() { return BOOLEAN; }
	string print() { 
		if (value)
			return "BOOL: true";
		else
			return "BOOL: false";
	}
private:
	bool value;
};

class ASTNodeString : public ASTNodeLiteral {
public:
	ASTNodeString(string s) : value(s) {}
	~ASTNodeString() {}

	NodeType type() { return STRING; }
	string print() { return "STRING: " + value; }
private:
	string value;
};

//----------------------------literal End-------------------------------------
//----------------------------statement Begin---------------------------------

class ASTNodeStatement : public ASTNode {
};

class ASTNodeBlock : public ASTNodeList {
public:
	ASTNodeBlock(ASTNodeStatement *stmt) {
		ASTNodeListConstruct(stmt);
	}
	ASTNodeBlock(ASTNodeBlock *blk, ASTNodeStatement *stmt) {
		if (blk == NULL || stmt == NULL)
			throw runtime_error("ASTNodeBlock: Constructor called with Nullptr!\n");
		ASTNodeListConstruct(blk, stmt);
	}
	~ASTNodeBlock() {}

	NodeType type() { return BLOCK; }
	string print() { return string("block") + (child ? "" : ": Empty block!"); }
};

//------------------Selection Statement-------------------------

class ASTNodeElifList : public ASTNode {
public:
	ASTNodeElifList(ASTNodeExpression *expr, ASTNodeBlock *blk) {
		if (expr == NULL || blk == NULL)
			throw runtime_error("ASTNodeElifList: Constructor called with Nullptr!\n");
		child = blk;
		addchild(expr);
	}
	ASTNodeElifList(ASTNodeElifList *elif, ASTNodeExpression *expr, ASTNodeBlock *blk) {
		if (elif == NULL || expr == NULL || blk == NULL)
			throw runtime_error("ASTNodeElifList: Constructor called with Nullptr!\n");
		child = elif->child;
		sibling = elif->sibling;
		ASTNode *tmp1 = child;
		ASTNode *tmp2 = child->getSibling();
		while (tmp2) {
			tmp1 = tmp2;
			tmp2 = tmp2->getSibling();
		}
		tmp1->setSibling(expr);
		expr->setSibling(blk);
	}
	~ASTNodeElifList() {}

	NodeType type() { return ELIF_LIST; }
	string print() { return "elif list"; }
};

class ASTNodeIfThenElseStmt : public ASTNodeStatement {
public:
	ASTNodeIfThenElseStmt(ASTNodeExpression *expr, ASTNodeBlock *blk1, ASTNodeBlock *blk2) {
		if (expr == NULL || blk1 == NULL)
			throw runtime_error("ASTNodeIfThenElseStmt: Constructor called with Nullptr!\n");
		if (blk2 == NULL)
			count = 2;
		else
			count = 3;
		child = blk2;
		addchild(blk1);
		addchild(expr);
	}
	ASTNodeIfThenElseStmt(ASTNodeExpression *expr, ASTNodeBlock *blk1, ASTNodeElifList *elif, ASTNodeBlock *blk2) {
		if (expr == NULL || blk1 == NULL || elif == NULL || blk2 == NULL)
			throw runtime_error("ASTNodeIfThenElseStmt: Constructor called with Nullptr!\n");
		count = 4;
		child = elif->getChild();
		sibling = elif->getSibling();
		ASTNode *tmp1 = child;
		ASTNode *tmp2 = child->getSibling();
		while (tmp2) {
			tmp1 = tmp2;
			tmp2 = tmp2->getSibling();
			count++;
		}
		tmp1->setSibling(blk2);
		addchild(blk1);
		addchild(expr);
	}
	~ASTNodeIfThenElseStmt() {}

	NodeType type() { return IF_THEN_ELSE_STMT; }
	string print() {
		if (count == 2)
			return "if then statement";
		else
			return "if then else statement";
	}

	void traverse_draw_terminal(int i, string prefix = "") {
		cout << string(i * 4, ' ') << "|-" << prefix << print() << endl;
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: if" << endl;
		child->traverse_draw_terminal(i + 1);
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: then" << endl;
		child->getSibling()->traverse_draw_terminal(i + 1);
		if (count == 2) {
			cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: end if" << endl;
			return;
		}

		ASTNode *tmp = child->getSibling()->getSibling();
		for (int ii = 3; ii < count; ii += 2) {
			cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: elif" << endl;
			tmp->traverse_draw_terminal(i + 1);
			tmp = tmp->getSibling();
			cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: then" << endl;
			tmp->traverse_draw_terminal(i + 1);
			tmp = tmp->getSibling();
		}
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: else" << endl;
		tmp->traverse_draw_terminal(i + 1);
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: end if" << endl;
	}
private:
	// count = 2: if then stmt
	// count = 3: if then else stmt
	// count > 3: if then (elif then)+ else stmt
	int count;
};

//------------------Iteration Statement-------------------------

class ASTNodeWhileStmt : public ASTNodeStatement {
public:
	ASTNodeWhileStmt(ASTNodeExpression *expr, ASTNodeBlock *blk) {
		if (expr == NULL || blk == NULL)
			throw runtime_error("ASTNodeWhileStmt: Constructor called with Nullptr!\n");
		child = blk;
		addchild(expr);
	}
	~ASTNodeWhileStmt() {}

	NodeType type() { return WHILE_STMT; }
	string print() { return "while statement"; }

	void traverse_draw_terminal(int i, string prefix = "") {
		cout << string(i * 4, ' ') << "|-" << prefix << print() << endl;
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: while" << endl;
		child->traverse_draw_terminal(i + 1);
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: do" << endl;
		child->getSibling()->traverse_draw_terminal(i + 1);
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: end while" << endl;
	}
};

class ASTNodeRepeatStmt : public ASTNodeStatement {
public:
	ASTNodeRepeatStmt(ASTNodeBlock *blk, ASTNodeExpression *expr) {
		if (blk == NULL || expr == NULL)
			throw runtime_error("ASTNodeRepeatStmt: Constructor called with Nullptr!\n");
		child = expr;
		addchild(blk);
	}
	~ASTNodeRepeatStmt() {}

	NodeType type() { return REPEAT_STMT; }
	string print() { return "repeat statement"; }

	void traverse_draw_terminal(int i, string prefix = "") {
		cout << string(i * 4, ' ') << "|-" << prefix << print() << endl;
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: repeat" << endl;
		child->traverse_draw_terminal(i + 1);
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: until" << endl;
		child->getSibling()->traverse_draw_terminal(i + 1);
	}
};

class ASTNodeForEachStmt : public ASTNodeStatement {
public:
	ASTNodeForEachStmt(ASTNodeID *id, ASTNodeExpression *expr, ASTNodeBlock *blk) {
		if (id == NULL || expr == NULL || blk == NULL)
			throw runtime_error("ASTNodeForEachStmt: Constructor called with Nullptr!\n");
		child = blk;
		addchild(expr);
		addchild(id);
	}
	~ASTNodeForEachStmt() {}

	NodeType type() { return FOREACH_STMT; }
	string print() { return "foreach statement"; }

	void traverse_draw_terminal(int i, string prefix = "") {
		cout << string(i * 4, ' ') << "|-" << prefix << print() << endl;
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: foreach" << endl;
		child->traverse_draw_terminal(i + 1);
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: in" << endl;
		child->getSibling()->traverse_draw_terminal(i + 1);
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: do" << endl;
		child->getSibling()->getSibling()->traverse_draw_terminal(i + 1);
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: end foreach" << endl;
	}
};

//-------------------------Others-------------------------------

class ASTNodeBreakStmt : public ASTNodeStatement {
public:
	NodeType type() { return BREAK_STMT; }
	string print() { return "break statement"; }
};

class ASTNodeContinueStmt : public ASTNodeStatement {
public:
	NodeType type() { return CONTINUE_STMT; }
	string print() { return "continue statement"; }
};

class ASTNodeReturnStmt : public ASTNodeStatement {
public:
	ASTNodeReturnStmt(ASTNodeExpression *expr) { child = expr; }
	~ASTNodeReturnStmt() {}

	NodeType type() { return RETURN_STMT; }
	string print() { return "return statement"; }
};

class ASTNodePrintStmt : public ASTNodeStatement {
public:
	ASTNodePrintStmt(ASTNodeExpression *expr) {
		if (expr == NULL)
			throw runtime_error("ASTNodePrintStmt: Constructor called with Nullptr!\n");
		child = expr;
	}
	~ASTNodePrintStmt() {}

	NodeType type() { return PRINT_STMT; }
	string print() { return "print statement"; }

	void traverse_draw_terminal(int i, string prefix = "") {
		cout << string(i * 4, ' ') << "|-" << prefix << print() << endl;
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: print" << endl;
		child->traverse_draw_terminal(i + 1);
	}
};

class ASTNodeExpressionStmt : public ASTNodeStatement {
public:
	ASTNodeExpressionStmt(ASTNodeExpression *expr) { child = expr; }
	~ASTNodeExpressionStmt() {}

	NodeType type() {
		if (child)
			return EXPR_STMT;
		else
			return EMPTY_STMT;
	}
	string print() {
		if (child)
			return "expression statement";
		else
			return "empty statement";
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
	ASTNodeParameterList(ASTNodeID *id) {
		ASTNodeListConstruct(id);
	}
	ASTNodeParameterList(ASTNodeParameterList *lst, ASTNodeID *id) {
		if (lst == NULL || id == NULL)
			throw runtime_error("ASTNodeParameterList: Constructor called with Nullptr!\n");
		ASTNodeListConstruct(lst, id);
	}
	~ASTNodeParameterList() {}

	NodeType type() { return PARAM_LIST; }
	string print() { return string("parameter list") + (child ? "" : ": No param!"); }
};

class ASTNodeVariableDecl : public ASTNodeDeclaration {
public:
	ASTNodeVariableDecl (ASTNodeID *id, ASTNodeType *type) {
		if (id == NULL || type == NULL)
			throw runtime_error("ASTNodeVariableDecl: Constructor called with Nullptr!\n");
		child = type;
		addchild(id);
	}
	~ASTNodeVariableDecl() {}

	NodeType type() { return VARIABLE_DECL; }
	string print() { return "variable declaration";}

	void traverse_draw_terminal(int i, string prefix = "") {
		cout << string(i * 4, ' ') << "|-" << prefix << print() << endl;
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: var" << endl;
		child->traverse_draw_terminal(i + 1);
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: is" << endl;
		child->getSibling()->traverse_draw_terminal(i + 1);
	}
};

class ASTNodeVariableDeclList : public ASTNodeDeclList {
public:
	ASTNodeVariableDeclList(ASTNodeVariableDecl *decl) {
		ASTNodeListConstruct(decl);
	}
	ASTNodeVariableDeclList(ASTNodeVariableDeclList *lst, ASTNodeVariableDecl *decl) {
		if (lst == NULL || decl == NULL)
			throw runtime_error("ASTNodeVariableDeclList: Constructor called with Nullptr!\n");
		ASTNodeListConstruct(lst, decl);
	}
	~ASTNodeVariableDeclList() {}

	NodeType type() { return VARIABLE_DECL_LIST; }
	string print() { return string("variable declaration list") + (child ? "" : ": No decl!"); }
};

class ASTNodeFunctionDecl : public ASTNodeDeclaration {
public:
	ASTNodeFunctionDecl(ASTNodeID *id, ASTNodeParameterList *param_list,
			ASTNodeVariableDeclList *param_decl, ASTNodeType *type) {
		if (id == NULL || param_list == NULL || param_decl == NULL || type == NULL)
			throw runtime_error("ASTNodeFunctionDecl: Constructor called with Nullptr!\n");
		child = type;
		addchild(param_decl);
		addchild(param_list);
		addchild(id);
	}
	~ASTNodeFunctionDecl() {}

	NodeType type() { return FUNC_DECL; }
	string print() { return "function declaration"; }

	void traverse_draw_terminal(int i, string prefix = "") {
		ASTNode *tmp = child;
		cout << string(i * 4, ' ') << "|-" << prefix << print() << endl;
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: function" << endl;
		tmp->traverse_draw_terminal(i + 1, "function name: ");
		tmp = tmp->getSibling();
		tmp->traverse_draw_terminal(i + 1, "function parameters: ");
		tmp = tmp->getSibling();
		tmp->traverse_draw_terminal(i + 1, "function parameter declarations: ");
		tmp = tmp->getSibling();
		if (dynamic_cast<ASTNodeType *>(tmp)->variableType() != ASTNodeType::VOID) {
			cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: return" << endl;
			tmp->traverse_draw_terminal(i + 1);
		}
		else
			cout << string(i * 4 + 4, ' ') << "|-" << "NO return value!" << endl;
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: end function" << endl;
	}
};

class ASTNodeFunctionDefn : public ASTNodeDeclaration {
public:
	ASTNodeFunctionDefn(ASTNodeID *id, ASTNodeParameterList *param_list,
			ASTNodeVariableDeclList *param_decl, ASTNodeType *type,
			ASTNodeVariableDeclList *local_decl, ASTNodeBlock *blk) {
		if (id == NULL || param_list == NULL || param_decl == NULL || type ==
				NULL || local_decl == NULL || blk == NULL)
			throw runtime_error("ASTNodeFunctionDecl: Constructor called with Nullptr!\n");
		child = blk;
		addchild(local_decl);
		addchild(type);
		addchild(param_decl);
		addchild(param_list);
		addchild(id);
	}
	~ASTNodeFunctionDefn() {}

	NodeType type() { return FUNC_DEFN; }
	string print() { return "function definition"; }

	void traverse_draw_terminal(int i, string prefix = "") {
		ASTNode *tmp = child;
		cout << string(i * 4, ' ') << "|-" << prefix << print() << endl;
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: function" << endl;
		tmp->traverse_draw_terminal(i + 1, "function name: ");
		tmp = tmp->getSibling();
		tmp->traverse_draw_terminal(i + 1, "function parameters: ");
		tmp = tmp->getSibling();
		tmp->traverse_draw_terminal(i + 1, "function parameter declarations: ");
		tmp = tmp->getSibling();
		if (dynamic_cast<ASTNodeType *>(tmp)->variableType() != ASTNodeType::VOID) {
			cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: return" << endl;
			tmp->traverse_draw_terminal(i + 1);
		}
		else
			cout << string(i * 4 + 4, ' ') << "|-" << "NO return value!" << endl;
		tmp = tmp->getSibling();
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: is" << endl;
		tmp->traverse_draw_terminal(i + 1, "function local variable declarations: ");
		tmp = tmp->getSibling();
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: begin" << endl;
		tmp->traverse_draw_terminal(i + 1, "function body: ");
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: end function" << endl;
	}
};

class ASTNodeArrayDecl : public ASTNodeDeclaration {
public:
	ASTNodeArrayDecl(ASTNodeID *id, ASTNodeExpression *expr, ASTNodeType *type) {
		if (id == NULL || expr == NULL || type == NULL)
			throw runtime_error("ASTNodeFunctionDecl: Constructor called with Nullptr!\n");
		child = type;
		addchild(expr);
		addchild(id);
	}
	~ASTNodeArrayDecl() {}

	NodeType type() { return ARRAY_DECL; }
	string print() { return "array declaration: "; }

	void traverse_draw_terminal(int i, string prefix = "") {
		cout << string(i * 4, ' ') << "|-" << prefix << print() << endl;
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: type" << endl;
		child->traverse_draw_terminal(i + 1);
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: is array of" << endl;
		child->getSibling()->traverse_draw_terminal(i + 1, "array length: ");
		child->getSibling()->getSibling()->traverse_draw_terminal(i + 1, "array type: ");
	}
private:
};

class ASTNodeClassBody : public ASTNodeDeclList {
public:
	ASTNodeClassBody(ASTNodeVariableDecl *decl) {
		ASTNodeListConstruct(decl);
	}
	ASTNodeClassBody(ASTNodeFunctionDefn *defn) {
		ASTNodeListConstruct(defn);
	}
	ASTNodeClassBody(ASTNodeClassBody *body, ASTNodeVariableDecl *decl) {
		if (body == NULL || decl == NULL)
			throw runtime_error("ASTNodeClassBody: Constructor called with Nullptr!\n");
		ASTNodeListConstruct(body, decl);
	}
	ASTNodeClassBody(ASTNodeClassBody *body, ASTNodeFunctionDefn *defn) {
		if (body == NULL || defn == NULL)
			throw runtime_error("ASTNodeClassBody: Constructor called with Nullptr!\n");
		ASTNodeListConstruct(body, defn);
	}
	~ASTNodeClassBody() {}

	NodeType type() { return CLASS_BODY; }
	string print() { return string("class body: ") + (child ? "" : ": Empty class body!"); }
};

class ASTNodeClassDecl : public ASTNodeDeclaration {
public:
	ASTNodeClassDecl(ASTNodeID *id, ASTNodeType *super, ASTNodeClassBody *body) {
		if (id == NULL || super == NULL || body == NULL)
			throw runtime_error("ASTNodeClassDecl: Constructor called with Nullptr!\n");
		child = body;
		addchild(super);
		addchild(id);
	}
	~ASTNodeClassDecl() {}

	NodeType type() { return CLASS_DECL; }
	string print() { return "class declaration: "; }

	void traverse_draw_terminal(int i, string prefix = "") {
		cout << string(i * 4, ' ') << "|-" << prefix << print() << endl;
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: type" << endl;
		child->traverse_draw_terminal(i + 1, "class name: ");
		if (dynamic_cast<ASTNodeType *>(child->getSibling())->variableType() != ASTNodeType::VOID) {
			cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: extends" << endl;
			child->getSibling()->traverse_draw_terminal(i + 1);
		}
		else
			cout << string(i * 4 + 4, ' ') << "|-" << "NO super class!" << endl;
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: is class" << endl;
		child->getSibling()->getSibling()->traverse_draw_terminal(i + 1);
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: end class" << endl;
	}
};

class ASTNodeCompoundDeclList : public ASTNodeDeclList {
public:
	ASTNodeCompoundDeclList(ASTNodeDeclaration *decl) {
		ASTNodeListConstruct(decl);
	}
	ASTNodeCompoundDeclList(ASTNodeCompoundDeclList *lst, ASTNodeDeclaration *decl) {
		if (lst == NULL || decl == NULL)
			throw runtime_error("ASTNodeCompoundDeclList: Constructor called with Nullptr!\n");
		ASTNodeListConstruct(lst, decl);
	}
	~ASTNodeCompoundDeclList() {}

	NodeType type() { return COMPOUND_DECL_LIST; }
	string print() { return string("global declarations: ") + (child ? "" : ": No decl!"); }
};

//------------------------------declaration End--------------------------------

class ASTNodeProgram : public ASTNode {
public:
	ASTNodeProgram(ASTNodeID *id, ASTNodeCompoundDeclList *cmpd_decl,
			ASTNodeVariableDeclList *local_decl, ASTNodeBlock *blk) {
		if (id == NULL || cmpd_decl == NULL || local_decl == NULL || blk == NULL)
			throw runtime_error("ASTNodeProgram: Constructor called with Nullptr!\n");
		child = blk;
		addchild(local_decl);
		addchild(cmpd_decl);
		addchild(id);
	}
	~ASTNodeProgram() {}

	NodeType type() { return PROGRAM; }
	string print() { return "program: "; }

	void traverse_draw_terminal(int i, string prefix = "") {
		ASTNode *tmp = child;
		cout << string(i * 4, ' ') << "|-" << prefix << print() << endl;
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: program" << endl;
		tmp->traverse_draw_terminal(i + 1, "program name: ");
		tmp = tmp->getSibling();
		tmp->traverse_draw_terminal(i + 1);
		tmp = tmp->getSibling();
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: is" << endl;
		tmp->traverse_draw_terminal(i + 1);
		tmp = tmp->getSibling();
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: begin" << endl;
		tmp->traverse_draw_terminal(i + 1, "program body: ");
		cout << string(i * 4 + 4, ' ') << "|-" << "KEYWORD: end" << endl;
	}
};
