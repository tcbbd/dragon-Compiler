#include <queue>
#include <set>
#include <cctype>
#include "ast.h"

static map<string, ASTNodeArrayDecl*> array_table;
static map<string, pair<ASTNodeType*, ASTNodeClassBody*>> class_table;
static map<string, ASTNodeFunctionDefn*> g_func_table;

static int str_count = 0;
static map<string, int> str_table;

static const char BREAK_FLAG = '\x80';
static const char CONTINUE_FLAG = '\x81';
static const char HOLE_WIDTH = 4;

string ASTNodeType::getTypeAsm(bool array_ref) {
	if (variable_type == VariableType::INTEGER ||
			variable_type == VariableType::BOOLEAN)
		return asm_str;
	else if (variable_type == VariableType::VOID)
		return "void";
	else if (array_table.find(value) != array_table.end()) {
		string ret = dynamic_cast<ASTNodeType*>(array_table[value]->getChildren()[2])->getAsm();
		if (array_ref)
			ret += "*";
		return ret;
	}
	else if (class_table.find(value) != class_table.end())
		return "%class." + value;
	else
		return "";
}

void ASTNodeProgram::collect_info() {
	str_table["\n"] = 0;
	str_table[" "] = 1;
	str_table["%d"] = 2;
	str_count = 3;
	ASTNode::collect_info();
}

void ASTNodeString::collect_info() {
	if (str_table.find(value) == str_table.end()) {
		str_table[value] = str_count;
		str_count++;
	}
}

pair<bool, int> ASTNodeBinaryExpr::eval() {
	if (op == ":=")
		return make_pair(false, 0);
	auto expr1 = dynamic_cast<ASTNodeExpression*>(children[0])->eval();
	if (!expr1.first)
		return make_pair(false, 0);
	auto expr2 = dynamic_cast<ASTNodeExpression*>(children[1])->eval();
	if (!expr2.first)
		return make_pair(false, 0);
	if (op == "or")
		return make_pair(true, bool(expr1.second) || bool(expr2.second));
	else if (op == "and")
		return make_pair(true, bool(expr1.second) && bool(expr2.second));
	else if (op == "|")
		return make_pair(true, expr1.second | expr2.second);
	else if (op == "^")
		return make_pair(true, expr1.second ^ expr2.second);
	else if (op == "&")
		return make_pair(true, expr1.second & expr2.second);
	else if (op == "==")
		return make_pair(true, int(expr1.second == expr2.second));
	else if (op == "!=")
		return make_pair(true, int(expr1.second != expr2.second));
	else if (op == "<=")
		return make_pair(true, int(expr1.second <= expr2.second));
	else if (op == ">=")
		return make_pair(true, int(expr1.second >= expr2.second));
	else if (op == "<")
		return make_pair(true, int(expr1.second < expr2.second));
	else if (op == ">")
		return make_pair(true, int(expr1.second > expr2.second));
	else if (op == "<<")
		return make_pair(true, expr1.second << expr2.second);
	else if (op == ">>")
		return make_pair(true, expr1.second >> expr2.second);
	else if (op == "+")
		return make_pair(true, expr1.second + expr2.second);
	else if (op == "-")
		return make_pair(true, expr1.second - expr2.second);
	else if (op == "*")
		return make_pair(true, expr1.second * expr2.second);
	else if (op == "/" || op == "%") {
		if (expr2.second == 0) {
			stringstream ss;
			ss << loc << " error: divide by zero" << endl;
			throw runtime_error(ss.str());
		}
		if (op == "/")
			return make_pair(true, expr1.second / expr2.second);
		if (op == "%")
			return make_pair(true, expr1.second % expr2.second);
	}
	return make_pair(false, 0);
}

void ASTNodeArrayDecl::collect_info() {
	string id = dynamic_cast<ASTNodeID*>(children[0])->getID();
	stringstream ss;
	if ((array_table.find(id) != array_table.end()) ||
			(class_table.find(id) != class_table.end())) {
		ss << loc << " error: redeclaration of array type '" << id << "'" << endl;
		throw runtime_error(ss.str());
	}
	auto length = dynamic_cast<ASTNodeExpression*>(children[1])->eval();
	if (!length.first) {
		ss << loc << " error: array length of array type '" << id << "' is not a constant expression" << endl;
		throw runtime_error(ss.str());
	}
	if (length.second <= 0) {
		ss << loc << " error: array length of array type '" << id << "' is not positive" << endl;
		throw runtime_error(ss.str());
	}
	this->length = length.second;
	array_table[id] = this;
}

void ASTNodeClassDecl::collect_info() {
	string id = dynamic_cast<ASTNodeID*>(children[0])->getID();
	stringstream ss;
	ASTNodeClassBody* superclass_body;
	if ((class_table.find(id) != class_table.end()) ||
			(array_table.find(id) != array_table.end())) {
		ss << loc << " error: redeclaration of class type '" << id << "'" << endl;
		throw runtime_error(ss.str());
	}
	ASTNodeType *super = dynamic_cast<ASTNodeType*>(children[1]);
	if ((super->variableType() == ASTNodeType::INTEGER) ||
			(super->variableType() == ASTNodeType::BOOLEAN)) {
		ss << loc << " error: super class of class type '" << id
			<< "' is declared to be " << super->getValue() << endl;
		throw runtime_error(ss.str());
	}
	if (super->variableType() != ASTNodeType::VOID) {
		if (array_table.find(super->getValue()) != array_table.end()) {
			ss << loc << " error: super class of class type '" << id
				<< "' is declared to be array type '" << super->getValue() << "'" << endl;
			throw runtime_error(ss.str());
		}
		auto iter = class_table.find(super->getValue());
		if (iter == class_table.end()) {
			ss << loc << " error: super class of class type '" << id
				<< "' is declared to be undeclared type '" << super->getValue() << "'" << endl;
			throw runtime_error(ss.str());
		}
		superclass_body = dynamic_cast<ASTNodeClassBody*>((*iter).second.second);
	}
	dynamic_cast<ASTNodeClassBody*>(children[2])->collect_info();
	if (super->variableType() != ASTNodeType::VOID)
		dynamic_cast<ASTNodeClassBody*>(children[2])->merge(superclass_body);
	class_table[id] = make_pair(super, dynamic_cast<ASTNodeClassBody*>(children[2]));
}

void ASTNodeClassBody::merge(ASTNodeClassBody* super) {
	stringstream ss;
	map<string, pair<int, ASTNodeType*>>* s_var_table = super->getVarTable();
	for (auto i : (*s_var_table)) {
		if (var_table.find(i.first) != var_table.end()) {
			ss << var_table[i.first].second->getLoc() << " error: class member '" << i.first
				<< "' will shadow the member of the same name in super class" << endl
				<< "(And you can never visit that member of super class from the derived class"
				<< " since we don't provide Java-like 'super' in MyLang)" << endl;
			throw runtime_error(ss.str());
		}
	}
}

void ASTNodeClassBody::collect_info() {
	stringstream ss;
	int var_count = 0;
	for (int i = 0; i < children.size(); ++i) {
		ASTNode* child = children[i];
		string id = dynamic_cast<ASTNodeID*>(child->getChildren()[0])->getID();
		if (child->type() == ASTNode::VARIABLE_DECL) {
			if (var_table.find(id) != var_table.end()) {
				ss << child->getLoc() << " error: redeclaration of member variable '"
					<< id << "'" << endl;
				throw runtime_error(ss.str());
			}
			if (func_table.find(id) != func_table.end()) {
				ss << child->getLoc() << " error: member variable '" << id
					<< "' conflicts with a previous declared member function" << endl;
				throw runtime_error(ss.str());
			}
			var_table[id] = make_pair(var_count, dynamic_cast<ASTNodeType*>(child->getChildren()[1]));
			var_count++;
		}
		else {
			if (var_table.find(id) != var_table.end()) {
				ss << child->getLoc() << " error: member function '" << id
					<< "' conflicts with a previous declared member variable" << endl;
				throw runtime_error(ss.str());
			}
			dynamic_cast<ASTNodeFunctionDefn*>(child)->collect_info(func_table);
		}
	}
}

void ASTNodeFunctionDefn::collect_info() { collect_info(g_func_table); }

void ASTNodeFunctionDefn::collect_info(map<string, ASTNodeFunctionDefn*> &func_table) {
	string id = dynamic_cast<ASTNodeID*>(children[0])->getID();
	stringstream ss;
	if (func_table.find(id) != func_table.end()) {
		ss << loc << " error: redefinition of function '" << id << "'" << endl;
		throw runtime_error(ss.str());
	}
	vector<ASTNode*>& param_list = children[1]->getChildren();
	vector<ASTNode*>& param_decl = children[2]->getChildren();
	if (param_list.size() != param_decl.size()) {
		ss << children[2]->getLoc() << " error: parameter declaration of function '" << id
			<< "' is not consistent" << endl;
		throw runtime_error(ss.str());
	}
	for (int i = 0; i < param_decl.size(); ++i) {
		string param_id = dynamic_cast<ASTNodeID*>(param_decl[i]->getChildren()[0])->getID();
		if (params.find(param_id) != params.end()) {
			ss << param_decl[i]->getLoc() << " error: parameter redeclared in function '" << id
				<< "'" << endl;
			throw runtime_error(ss.str());
		}
		params[param_id] = make_pair(i, dynamic_cast<ASTNodeType*>(param_decl[i]->getChildren()[1]));
	}
	for (int i = 0; i < param_list.size(); ++i) {
		string param_id = dynamic_cast<ASTNodeID*>(param_list[i])->getID();
		if (params.find(param_id) != params.end()) {
			params[param_id].first = i;
		}
		else {
			ss << children[2]->getLoc() << " error: parameter '" << param_id
				<< "' is not declared in function '" << id << "'" << endl;
			throw runtime_error(ss.str());
		}
	}

	vector<ASTNode*>& local_decl = children[4]->getChildren();
	for (int i = 0; i < local_decl.size(); ++i) {
		string local_id = dynamic_cast<ASTNodeID*>
			(dynamic_cast<ASTNodeVariableDecl*>(local_decl[i])->getChildren()[0])->getID();
		if (localvar_table.find(local_id) != localvar_table.end()) {
			ss << local_decl[i]->getLoc() << " error: redeclaration of local variable '"
				<< local_id << "'" << endl;
			throw runtime_error(ss.str());
		}
		if (params.find(local_id) != params.end()) {
			ss << local_decl[i]->getLoc() << " error: declaration of local variable '"
				<< local_id << "' conflicts with a parameter" << endl;
			throw runtime_error(ss.str());
		}
		localvar_table[local_id] = dynamic_cast<ASTNodeType*>(local_decl[i]->getChildren()[1]);
	}

	func_table[id] = this;
	children[5]->collect_info();
}

//-----------------------------------------------------------------------

struct GenCodeInfo {
	GenCodeInfo(string class_id, map<string, ASTNodeFunctionDefn*>* func_table,
			map<string, pair<int, ASTNodeType*>>* var_table,
			map<string, pair<int, ASTNodeType*>>* params,
			map<string, ASTNodeType*>* localvar_table,
			ASTNodeType* ret_type, int count) {
		this->class_id = class_id;
		this->func_table = func_table;
		this->var_table = var_table;
		this->params = params;
		this->localvar_table = localvar_table;
		this->ret_type = ret_type;

		tempval_count = count;
		current_block = 0;
		in_loop = false;
		block_isover = false;
		terminated_bybr = true;

		result_type = NONE;
	}

	enum ResultType {
		SIMPLE, // ID, THIS, INTEGER, BOOLEAN, STRING
		VALUE,
		POINTER,
		FUNCTION,
		NONE
	};
	string class_id;
	map<string, ASTNodeFunctionDefn*>* func_table;
	map<string, pair<int, ASTNodeType*>>* var_table;
	map<string, pair<int, ASTNodeType*>>* params;
	map<string, ASTNodeType*>* localvar_table;
	ASTNodeType* ret_type;

	int tempval_count;
	int current_block;
	bool in_loop;
	bool block_isover;
	bool terminated_bybr;
	vector<int> break_point;
	vector<int> continue_point;

	ResultType result_type;
	struct Result {
		ASTNodeExpression* expr;
		struct {
			int index; // %1
			string id; // %local, when index = -1
			ASTNodeType* type;
			bool islvalue;
		} regval;
		struct {
			string class_id;
			int this_index; // %1
			string this_id; // %local, when this_index = -1
			ASTNodeFunctionDefn* func;
		} func;
	} result;
	yy::location loc;
};

void ASTNodeProgram::gen_code() {
	stringstream ss;
	cout << "target datalayout = \"e-m:e-i64:64-f80:128-n8:16:32:64-S128\"" << endl;
	cout << "target triple = \"x86_64-pc-linux-gnu\"" << endl;
	cout << endl;

	gen_typedef();
	cout << endl;

	// gen_string
	cout.fill('0');
	for(auto iter : str_table) {
		cout << "@.str" << iter.second << " = private unnamed_addr constant ["
			<< (iter.first.size() + 1) << " x i8] c\"";
		cout.setf(ios::hex, ios::basefield);
		cout.setf(ios::uppercase);
		for (int i = 0; i < iter.first.size(); ++i) {
			if (iscntrl(iter.first[i]) || iter.first[i] == '"' || iter.first[i] == '\\') {
				cout << "\\";
				cout.width(2);
				cout << int((unsigned char)(iter.first[i]));
			}
			else
				cout << iter.first[i];
		}
		cout.unsetf(ios::basefield | ios::uppercase);
		cout << "\\00\", align 1" << endl;
	}
	cout.fill(' ');
	cout << endl;

	// class functions
	for (auto i : class_table)
		i.second.second->gen_code(i.first);

	// global functions
	for(auto i : g_func_table)
		i.second->gen_code();

	// main()
	vector<ASTNode*>& local_decl = children[2]->getChildren();
	for (int i = 0; i < local_decl.size(); ++i) {
		string local_id = dynamic_cast<ASTNodeID*>
			(dynamic_cast<ASTNodeVariableDecl*>(local_decl[i])->getChildren()[0])->getID();
		if (localvar_table.find(local_id) != localvar_table.end()) {
			ss << local_decl[i]->getLoc() << " error: redeclaration of local variable '"
				<< local_id << "'" << endl;
			throw runtime_error(ss.str());
		}
		localvar_table[local_id] = dynamic_cast<ASTNodeType*>(local_decl[i]->getChildren()[1]);
	}
	// we never use argc and argv, so hide it through the prefix "..."
	cout << "define i32 @main(i32 %...argc, i8** %...argv) #2 {" << endl;
	for (auto i : localvar_table) {
		string s = i.second->getTypeAsm(false);
		if (s.empty()) {
			ss << i.second->getLoc() << " error: function local variable '" << i.first
				<< "' is of type '" << i.second->getValue() << "' which is undelcared" << endl;
			throw runtime_error(ss.str());
		}
		cout << "  %" << i.first << " = alloca " << s << ", align 4" << endl;
	}

	ASTNodeType *ret_type = new ASTNodeType(ASTNodeType::INTEGER);
	GenCodeInfo gen_code_info("", NULL, NULL, NULL, &localvar_table, ret_type, 1);
	cout << dynamic_cast<ASTNodeBlock*>(children[3])->gen_code(&gen_code_info);
	delete ret_type;
	if (!gen_code_info.block_isover) {
		cout << "  ret i32 0" << endl;
	}
	else if (gen_code_info.terminated_bybr)
		cout << "  unreachable" << endl;
	cout << "}" << endl;

	cout << endl;
	cout << "declare i32 @printf(i8*, ...) #0" << endl;
	cout << endl;
	cout << R"(attributes #0 = { "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" })" << endl;
	cout << R"(attributes #1 = { uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" })" << endl;
	cout << R"(attributes #2 = { nounwind uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" })" << endl;
}

//----------------------Type Definition----------------------------

void ASTNodeProgram::gen_typedef() {
	stringstream ss;
	// construct the graph
	for (auto i : array_table) {
		ASTNodeType* type = dynamic_cast<ASTNodeType*>(i.second->getChildren()[2]);
		string var;
		if ((type->variableType() == ASTNodeType::INTEGER) ||
				(type->variableType() == ASTNodeType::BOOLEAN)) {
			var = type->getAsm();
		}
		else {
			if (array_table.find(type->getValue()) != array_table.end()) {
				i.second->depend(array_table[type->getValue()]);
				continue;
			}
			else if (class_table.find(type->getValue()) != class_table.end()) {
				var = "%class." + type->getValue();
			}
			else {
				ss << type->getLoc() << " error: array '" << i.first << "' is of type '"
					<< type->getValue() << "' which is undeclared" << endl;
				throw runtime_error(ss.str());
			}
		}
		ss << "[" << i.second->getLength() << " x " << var << "]";
		type->setAsm(ss.str());
		ss.str("");
	}
	// topology sort
	set<ASTNodeArrayDecl*> array_graph;
	queue<ASTNodeArrayDecl*> array_queue;
	for (auto i : array_table) {
		array_graph.insert(i.second);
		if (i.second->getIndegree() == 0)
			array_queue.push(i.second);
	}
	while (!array_queue.empty()) {
		ASTNodeArrayDecl* vertex = array_queue.front();
		vector<ASTNodeArrayDecl*>& dependers = vertex->getDepender();
		for (ASTNodeArrayDecl* depender : dependers) {
			ss << "[" << depender->getLength() << " x "
				<< dynamic_cast<ASTNodeType*>(vertex->getChildren()[2])->getAsm() << "]";
			dynamic_cast<ASTNodeType*>(depender->getChildren()[2])->setAsm(ss.str());
			ss.str("");
			depender->decreaseIndegree();
			if (depender->getIndegree() == 0)
				array_queue.push(depender);
		}
		array_graph.erase(vertex);
		array_queue.pop();
	}
	if (!array_graph.empty()) {
		ASTNodeArrayDecl *begin, *i;
		i = begin = *array_graph.begin();
		ss << "error: there is circular array declaration" << endl;
		string id, begin_id;
		id = begin_id = dynamic_cast<ASTNodeID*>(i->getChildren()[0])->getID();
		string type = dynamic_cast<ASTNodeType*>(i->getChildren()[2])->getValue();
		while (type != begin_id) {
			ss << i->getLoc() << " array '"<< id << "' is of type '" << type << "'" << endl;
			i = array_table[type];
			id = type;
			type = dynamic_cast<ASTNodeType*>(i->getChildren()[2])->getValue();
		}
		ss << i->getLoc() << " array '"<< id << "' is of type '" << type << "'" << endl;
		throw runtime_error(ss.str());
	}

	// construct the graph
	for (auto i : class_table) {
		map<string, pair<int, ASTNodeType*>>* var_table = i.second.second->getVarTable();
		vector<string> vars(var_table->size());
		for (auto j : (*var_table)) {
			ASTNodeType* type = j.second.second;
			if ((type->variableType() == ASTNodeType::INTEGER) ||
					(type->variableType() == ASTNodeType::BOOLEAN))
				vars[j.second.first] = type->getAsm();
			else {
				if (array_table.find(type->getValue()) != array_table.end()) {
					vars[j.second.first] = dynamic_cast<ASTNodeType*>
						(array_table[type->getValue()]->getChildren()[2])->getAsm();
				}
				else if (class_table.find(type->getValue()) != class_table.end()) {
					vars[j.second.first] = "%class." + type->getValue();
					ss << type->getLoc() << " var '" << j.first << "' is of type '"
						<< type->getValue() << "'" << endl;
					i.second.second->depend(ss.str(), class_table[type->getValue()].second);
					ss.str("");
				}
				else {
					ss << type->getLoc() << " error: variable '" << j.first << "' is of type '"
						<< type->getValue() << "' which is undeclared" << endl;
					throw runtime_error(ss.str());
				}
			}
		}
		ss << "%class." << i.first << " = type { ";
		// make an empty class really take some space, so that it's easier to be derived
		if ((i.second.first->variableType() == ASTNodeType::VOID) && vars.empty())
			ss << "i8";

		if (i.second.first->variableType() != ASTNodeType::VOID) {
			ss << "%class." << i.second.first->getValue();
			stringstream sss;
			sss << i.second.first->getLoc() << " class '" << i.first
				<< "' extends '" << i.second.first->getValue() << "'" << endl;
			i.second.second->depend(sss.str(), class_table[i.second.first->getValue()].second);
			if (!vars.empty())
				ss << ", ";
			// set the correct index
			for (auto iter = var_table->begin(); iter != var_table->end(); iter++)
				iter->second.first++;
		}
		if (!vars.empty())
			ss << vars[0];
		for (int i = 1; i < vars.size(); ++i)
			ss << ", " << vars[i];
		ss << " }" << endl;
		i.second.second->setAsm(ss.str());
		ss.str("");
	}
	// topology sort
	set<ASTNodeClassBody*> class_graph;
	queue<ASTNodeClassBody*> class_queue;
	for (auto i : class_table) {
		class_graph.insert(i.second.second);
		if (i.second.second->getIndegree() == 0)
			class_queue.push(i.second.second);
	}
	while (!class_queue.empty()) {
		ASTNodeClassBody* vertex = class_queue.front();
		cout << vertex->getAsm() << endl;
		vector<pair<string, ASTNodeClassBody*>>& dependers = vertex->getDepender();
		for (auto depender : dependers) {
			depender.second->decreaseIndegree();
			if (depender.second->getIndegree() == 0)
				class_queue.push(depender.second);
		}
		class_graph.erase(vertex);
		class_queue.pop();
	}
	if (!class_graph.empty()) {
		stringstream sss;
		sss << "error: there is circular class declaration" << endl;
		(*class_graph.begin())->dfs()->printinfo(sss);
		throw runtime_error(sss.str());
	}
}

//---------------------Function Definition-------------------------

void ASTNodeFunctionDefn::gen_code(string class_id, ASTNodeClassBody *class_body) {
	stringstream ss;
	cout << "define ";
	// define <ret type>
	ASTNodeType *ret_type = dynamic_cast<ASTNodeType*>(children[3]);
	string ret_asm = ret_type->getTypeAsm(false);
	if (!ret_asm.empty()) {
		if (ret_asm[0] != '[')
			cout << ret_asm;
		else {
			ss << ret_type->getLoc() << " error: array type '"<< ret_type->getValue()
				<< "' is not allowed to be return value of function" << endl;
			throw runtime_error(ss.str());
		}
	}
	else {
		ss << ret_type->getLoc() << " error: return type '"<< ret_type->getValue()
			<< "' of function '" << dynamic_cast<ASTNodeID*>(children[0])->getID()
			<< "' is not declared" << endl;
		throw runtime_error(ss.str());
	}

	// @func(type1 %param1, type2 %param2, ...)
	cout << " @";
	string func_name = dynamic_cast<ASTNodeID*>(children[0])->getID();
	if (!class_id.empty())
		cout << "class." << class_id << "." << func_name << "(";
	else {
		// we should prevent global functions from conflicting with main()
		if (func_name == "main")
			cout << "...main(";
		else
			cout << func_name << "(";
	}
	vector<string> paramstr(params.size());
	for (auto i : params) {
		string s = i.second.second->getTypeAsm(true);
		if (!s.empty()) {
			paramstr[i.second.first] = s + " %" + i.first;
		}
		else {
			ss << i.second.second->getLoc() << " error: function parameter '" << i.first
				<< "' is of type '" << i.second.second->getValue() << "' which is undelcared" << endl;
			throw runtime_error(ss.str());
		}
	}
	if (!class_id.empty()) {
		cout << "%class." << class_id << "* %this";
		if (!paramstr.empty())
			cout << ", ";
	}
	if (!paramstr.empty())
		cout << paramstr[0];
	for (int i = 1; i < paramstr.size(); ++i)
		cout << ", " << paramstr[i];
	// #1 is function attribute uwtable
	cout << ") #1 {" << endl;

	// parameters
	int delta = 1;
	if (!class_id.empty()) {
		delta = 2;
		cout << "  %1 = alloca %class." << class_id << "*, align 4" << endl;
	}
	for (auto i : params) {
		stringstream sss;
		sss << "  %" << (i.second.first + delta) << " = alloca "
			<< i.second.second->getTypeAsm(true) << ", align 4" << endl;
		paramstr[i.second.first] = sss.str();
	}
	for (string str : paramstr)
		cout << str;
	if (!class_id.empty())
		cout << "  store %class." << class_id << "* %this, "
			<< "%class." << class_id << "** %1, align 4" << endl;
	for (auto i : params) {
		stringstream sss;
		sss << "  store " << i.second.second->getTypeAsm(true) << " %" << i.first << ", "
			<< i.second.second->getTypeAsm(true) << "* %" << (i.second.first + delta)
			<< ", align 4" << endl;
		paramstr[i.second.first] = sss.str();
	}
	for (string str : paramstr)
		cout << str;

	// local variables
	for (auto i : localvar_table) {
		string s = i.second->getTypeAsm(false);
		if (s.empty()) {
			ss << i.second->getLoc() << " error: function local variable '" << i.first
				<< "' is of type '" << i.second->getValue() << "' which is undelcared" << endl;
			throw runtime_error(ss.str());
		}
		cout << "  %" << i.first << " = alloca " << s << ", align 4" << endl;
	}

	GenCodeInfo* gen_code_info;
	if (class_id.empty())
		gen_code_info = new GenCodeInfo(class_id, NULL, NULL,
				&params, &localvar_table, ret_type, params.size() + 1);
	else
		gen_code_info = new GenCodeInfo(class_id, class_body->getFuncTable(), class_body->getVarTable(),
				&params, &localvar_table, ret_type, params.size() + 2);
	cout << dynamic_cast<ASTNodeBlock*>(children[5])->gen_code(gen_code_info);
	if (!gen_code_info->block_isover) {
		if (ret_type->variableType() != ASTNodeType::VOID) {
			ss << loc << " error: control reaches end of non-void function" << endl;
			throw runtime_error(ss.str());
		}
		else
			cout << "  ret void" << endl;
	}
	else if (gen_code_info->terminated_bybr)
		cout << "  unreachable" << endl;
	cout << "}" << endl << endl;
	delete gen_code_info;
}

//-----------------------------Expressions---------------------------------

string ASTNodeExpression::gen_code(GenCodeInfo* gen_code_info) {
	gen_code_info->result_type = GenCodeInfo::SIMPLE;
	gen_code_info->result.expr = this;
	gen_code_info->loc = loc;
	return "";
}

static string find_id_byvar(GenCodeInfo* gen_code_info, string id, int &index, ASTNodeType **type) {
	stringstream ss, result;
	map<string, pair<int, ASTNodeType*>>* var_table = gen_code_info->var_table;
	result << "  %" << gen_code_info->tempval_count++ << " = load %class."
		<< gen_code_info->class_id << "** %1, align 4" << endl;
	result << "  %" << gen_code_info->tempval_count << " = getelementptr inbounds %class."
		<< gen_code_info->class_id << "* %" << gen_code_info->tempval_count - 1 << ", i32 0";
	index = gen_code_info->tempval_count++;

	if (var_table->find(id) != var_table->end()) {
		*type = (*var_table)[id].second;
		result << ", i32 " << (*var_table)[id].first << endl;
	}
	else {
		ASTNodeType *superclass = class_table[gen_code_info->class_id].first;
		bool failed = true;
		while (superclass->variableType() != ASTNodeType::VOID) {
			result << ", i32 0";
			var_table = class_table[superclass->getValue()].second->getVarTable();
			if (var_table->find(id) != var_table->end()) {
				*type = (*var_table)[id].second;
				result << ", i32 " << (*var_table)[id].first << endl;
				failed = false;
				break;
			}
			superclass = class_table[superclass->getValue()].first;
		}
		if (failed) {
			ss << gen_code_info->loc << " error: variable '" << id << "' is used before declared" << endl;
			throw runtime_error(ss.str());
		}
	}
	return result.str();
}

static string find_id(GenCodeInfo* gen_code_info, string id, int &index, string &index_id, ASTNodeType **type) {
	stringstream ss;
	string ret;
	if (gen_code_info->params &&
			gen_code_info->params->find(id) != gen_code_info->params->end()) {
		index = (*(gen_code_info->params))[id].first + 1;
		if (!gen_code_info->class_id.empty())
			index++;
		*type = (*(gen_code_info->params))[id].second;
	}
	else if (gen_code_info->localvar_table &&
			gen_code_info->localvar_table->find(id) != gen_code_info->localvar_table->end()) {
		index = -1;
		index_id = id;
		*type = (*(gen_code_info->localvar_table))[id];
	}
	else if (gen_code_info->var_table) {
		ret = find_id_byvar(gen_code_info, id, index, type);
	}
	else {
		ss << gen_code_info->loc << " error: variable '" << id << "' is used before declared" << endl;
		throw runtime_error(ss.str());
	}
	return ret;
}

static string load_id(GenCodeInfo* gen_code_info, ASTNodeExpression* expr, int &index, string *index_id, ASTNodeType **type) {
	// load array as pointer ([i x type]*)
	stringstream ss, result;
	string id = dynamic_cast<ASTNodeID*>(expr)->getID();
	if (gen_code_info->params &&
			gen_code_info->params->find(id) != gen_code_info->params->end()) {
		index = (*(gen_code_info->params))[id].first + 1;
		if (!gen_code_info->class_id.empty())
			index++;
		*type = (*(gen_code_info->params))[id].second;
		result << "  %" << gen_code_info->tempval_count << " = load "
			<< (*type)->getTypeAsm(true) << "* %" << index << ", align 4" << endl;
		index = gen_code_info->tempval_count++;
	}
	else if (gen_code_info->localvar_table &&
			gen_code_info->localvar_table->find(id) != gen_code_info->localvar_table->end()) {
		*type = (*(gen_code_info->localvar_table))[id];
		if (array_table.find((*type)->getValue()) != array_table.end()) {
			index = -1;
			if (index_id)
				*index_id = id;
		}
		else {
			result << "  %" << gen_code_info->tempval_count << " = load "
				<< (*type)->getTypeAsm(false) << "* %" << id << ", align 4" << endl;
			index = gen_code_info->tempval_count++;
		}
	}
	else if (gen_code_info->var_table) {
		result << find_id_byvar(gen_code_info, id, index, type);
		if (array_table.find((*type)->getValue()) == array_table.end()) {
			result << "  %" << gen_code_info->tempval_count << " = load "
				<< (*type)->getTypeAsm(false) << "* %" << index << ", align 4" << endl;
			index = gen_code_info->tempval_count++;
		}
	}
	else {
		ss << gen_code_info->loc << " error: variable '" << id << "' is used before declared" << endl;
		throw runtime_error(ss.str());
	}
	return result.str();
}

string ASTNodeFieldAccess::gen_asm(GenCodeInfo* gen_code_info, ASTNodeFieldAccess::LvalType lvaltype) {
	stringstream ss;
	string id;
	if (lvaltype == ID) {
		ASTNodeExpression* expr = gen_code_info->result.expr;
		id = dynamic_cast<ASTNodeID*>(expr)->getID();
	}
	string rid = dynamic_cast<ASTNodeID*>(children[1])->getID();

	// phase 1, get proper information
	int func_this_index;
	string func_this_id;
	ASTNodeType* type;
	stringstream pre_result, result;
	if (lvaltype == ID) {
		gen_code_info->result.regval.islvalue = true;
		pre_result << find_id(gen_code_info, id, func_this_index, func_this_id, &type);
		result << "  %" << gen_code_info->tempval_count << " = getelementptr inbounds %class."
			<< type->getValue() << "* %";
		if (func_this_index >= 0)
			result << func_this_index;
		else
			result << func_this_id;
		result << ", i32 0";
	}
	else if (lvaltype == THISPOINTER) {
		gen_code_info->result.regval.islvalue = true;
		// add to children so that it can be automatically deleted
		type = new ASTNodeType(gen_code_info->class_id);
		children.push_back(type);
		pre_result << "  %" << gen_code_info->tempval_count << " = load %class."
			<< gen_code_info->class_id << "** %1, align 4" << endl;
		func_this_index = gen_code_info->tempval_count++;
		result << "  %" << gen_code_info->tempval_count << " = getelementptr inbounds %class."
			<< gen_code_info->class_id << "* %" << func_this_index << ", i32 0";
	}
	else if (lvaltype == COMPOSED) {
		// islvalue will not change
		func_this_index = gen_code_info->result.regval.index;
		func_this_id = gen_code_info->result.regval.id;
		type = gen_code_info->result.regval.type;
		result << "  %" << gen_code_info->tempval_count << " = getelementptr inbounds %class."
			<< type->getValue() << "* %";
		if (func_this_index >= 0)
			result << func_this_index;
		// consider a = b.c(), where local variable a is of class A and b.c() returns class A
		// in this case left part of (a = b.c()).d should be accessed by %a
		else
			result << func_this_id;
		result << ", i32 0";
	}

	if (type->variableType() == ASTNodeType::INTEGER) {
		ss << gen_code_info->loc << " error: can't use operator '.' on integer" << endl;
		throw runtime_error(ss.str());
	}
	else if (type->variableType() == ASTNodeType::BOOLEAN) {
		ss << gen_code_info->loc << " error: can't use operator '.' on boolean" << endl;
		throw runtime_error(ss.str());
	}
	else if (array_table.find(type->getValue()) != array_table.end()) {
		ss << gen_code_info->loc << " error: can't use operator '.' on array type '" << type->getValue() << "'" << endl;
		throw runtime_error(ss.str());
	}
	else if (class_table.find(type->getValue()) == class_table.end())
		throw runtime_error("panic: unexpected code path, BUG in code!\n");

	// phase 2, generate the pointer
	map<string, pair<int, ASTNodeType*>>* param_var_table = class_table[type->getValue()].second->getVarTable();
	map<string, ASTNodeFunctionDefn*>* param_func_table = class_table[type->getValue()].second->getFuncTable();

	if (param_var_table->find(rid) != param_var_table->end()) {
		// direct member access
		result << ", i32 " << (*param_var_table)[rid].first << endl;
		gen_code_info->result_type = GenCodeInfo::POINTER;
		gen_code_info->result.regval.index = gen_code_info->tempval_count++;
		gen_code_info->result.regval.type = (*param_var_table)[rid].second;
		gen_code_info->loc = loc;
		return pre_result.str() + result.str();
	}
	else if (param_func_table->find(rid) != param_func_table->end()) {
		// direct function access
		gen_code_info->result_type = GenCodeInfo::FUNCTION;
		gen_code_info->result.func.class_id = type->getValue();
		gen_code_info->result.func.this_index = func_this_index;
		gen_code_info->result.func.this_id = func_this_id;
		gen_code_info->result.func.func = (*param_func_table)[rid];
		gen_code_info->loc = loc;
		return pre_result.str();
	}
	else {
		// super class member/function access
		ASTNodeType* superclass = class_table[type->getValue()].first;
		while (superclass->variableType() != ASTNodeType::VOID) {
			result << ", i32 0";
			param_var_table = class_table[superclass->getValue()].second->getVarTable();
			param_func_table = class_table[superclass->getValue()].second->getFuncTable();
			if (param_var_table->find(rid) != param_var_table->end()) {
				// super class member access
				result << ", i32 " << (*param_var_table)[rid].first << endl;
				gen_code_info->result_type = GenCodeInfo::POINTER;
				gen_code_info->result.regval.index = gen_code_info->tempval_count++;
				gen_code_info->result.regval.type = (*param_var_table)[rid].second;
				gen_code_info->loc = loc;
				return pre_result.str() + result.str();
			}
			else if (param_func_table->find(rid) != param_func_table->end()) {
				// super class function access
				result << endl;
				gen_code_info->result_type = GenCodeInfo::FUNCTION;
				gen_code_info->result.func.class_id = superclass->getValue();
				gen_code_info->result.func.this_index = gen_code_info->tempval_count++;
				gen_code_info->result.func.func = (*param_func_table)[rid];
				gen_code_info->loc = loc;
				return pre_result.str() + result.str();
			}
			superclass = class_table[superclass->getValue()].first;
		}

		if (!id.empty()) {
			ss << gen_code_info->loc << " error: variable '" << id
				<< "' has no member '" << rid << "'" << endl;
			throw runtime_error(ss.str());
		}
		else if (lvaltype == THISPOINTER) {
			ss << children[1]->getLoc() << " error: class member '" << rid << "' is used before declared" << endl;
			throw runtime_error(ss.str());
		}
		else if (lvaltype == COMPOSED) {
			ss << gen_code_info->loc << " error: invalid use of operator '.' "
				<< ": requested member doesn't exist" << endl;
			throw runtime_error(ss.str());
		}
	}
}

string ASTNodeFieldAccess::gen_composed(GenCodeInfo* gen_code_info) {
	stringstream result;
	if (gen_code_info->result_type == GenCodeInfo::VALUE) {
		string class_id = gen_code_info->result.regval.type->getValue();
		result << "  %" << gen_code_info->tempval_count << " = alloca %class."
			<< class_id << "*, align 4" << endl;
		// intermediate value will aways be anonymous, so we can index by int safely
		result << "  store %class." << class_id << " %" << gen_code_info->result.regval.index
			<< ", %class." << class_id << "* %" << gen_code_info->tempval_count << ", align 4" << endl;
		gen_code_info->result.regval.index = gen_code_info->tempval_count++;
	}
	result << gen_asm(gen_code_info, COMPOSED);
	return result.str();
}

string ASTNodeFieldAccess::gen_simple(GenCodeInfo* gen_code_info) {
	stringstream ss;
	ASTNodeExpression* expr = gen_code_info->result.expr;
	if (!expr) {
		throw runtime_error("panic: unexpected code path, BUG in code!\n");
	}
	if (expr->type() == ASTNode::IDENTIFIER)
		return gen_asm(gen_code_info, ID);
	else if (expr->type() == ASTNode::THIS) {
		if (gen_code_info->class_id.empty()) {
			ss << gen_code_info->loc << " error: invalid use of 'this' in non-member function" << endl;
			throw runtime_error(ss.str());
		}
		return gen_asm(gen_code_info, THISPOINTER);
	}
	else if (expr->type() == ASTNode::INTEGER) {
		ss << gen_code_info->loc << " error: can't use operator '.' on integer" << endl;
		throw runtime_error(ss.str());
	}
	else if (expr->type() == ASTNode::BOOLEAN) {
		ss << gen_code_info->loc << " error: can't use operator '.' on boolean" << endl;
		throw runtime_error(ss.str());
	}
	else if (expr->type() == ASTNode::STRING) {
		ss << gen_code_info->loc << " error: can't use operator '.' on string" << endl;
		throw runtime_error(ss.str());
	}
	else {
		ss << gen_code_info->loc << " panic: unexpected code path, BUG in code!" << endl;
		throw runtime_error(ss.str());
	}
}

string ASTNodeFieldAccess::gen_code(GenCodeInfo* gen_code_info) {
	stringstream ss;
	string ret = dynamic_cast<ASTNodeExpression*>(children[0])->gen_code(gen_code_info);
	if (gen_code_info->result_type == GenCodeInfo::NONE) {
		ss << gen_code_info->loc << " error: can't use operator '.' on 'void' value" << endl;
		throw runtime_error(ss.str());
	}
	else if (gen_code_info->result_type == GenCodeInfo::SIMPLE) {
		return gen_simple(gen_code_info);
	}
	else if (gen_code_info->result_type == GenCodeInfo::FUNCTION) {
		ss << gen_code_info->loc << " error: can't use operator '.' on function" << endl;
		throw runtime_error(ss.str());
	}
	else if (gen_code_info->result_type == GenCodeInfo::POINTER ||
			gen_code_info->result_type == GenCodeInfo::VALUE) {
		ASTNodeType* type = gen_code_info->result.regval.type;
		if (type->variableType() == ASTNodeType::INTEGER ||
				type->variableType() == ASTNodeType::BOOLEAN) {
			ss << gen_code_info->loc << " error: can't use operator '.' on " << type->getValue() << endl;
			throw runtime_error(ss.str());
		}
		else if (array_table.find(type->getValue()) != array_table.end()) {
			ss << gen_code_info->loc << " error: can't use operator '.' on array type '" << type->getValue() << "'" << endl;
			throw runtime_error(ss.str());
		}
		else if (class_table.find(type->getValue()) != class_table.end()) {
			return ret + gen_composed(gen_code_info);
		}
		else {
			ss << gen_code_info->loc << " panic: unexpected code path, BUG in code!" << endl;
			throw runtime_error(ss.str());
		}
	}
	return ret;
}

void ASTNodeArrayAccess::check_type(GenCodeInfo* gen_code_info, ASTNodeType* type) {
	stringstream ss;
	if (type->variableType() == ASTNodeType::INTEGER) {
		ss << gen_code_info->loc << " error: can't use operator '[]' on integer" << endl;
		throw runtime_error(ss.str());
	}
	else if (type->variableType() == ASTNodeType::BOOLEAN) {
		ss << gen_code_info->loc << " error: can't use operator '[]' on boolean" << endl;
		throw runtime_error(ss.str());
	}
	else if (class_table.find(type->getValue()) != class_table.end()) {
		ss << gen_code_info->loc << " error: can't use operator '[]' on class type '" << type->getValue() << "'" << endl;
		throw runtime_error(ss.str());
	}
	else if (array_table.find(type->getValue()) == array_table.end())
		throw runtime_error("panic: unexpected code path, BUG in code!\n");
}

string ASTNodeArrayAccess::gen_asm(GenCodeInfo* gen_code_info, ASTNodeFieldAccess::LvalType lvaltype) {
	stringstream ss;
	string id;
	if (lvaltype == ID) {
		ASTNodeExpression* expr = gen_code_info->result.expr;
		id = dynamic_cast<ASTNodeID*>(expr)->getID();
	}

	string ret;
	stringstream pre_result, result;
	ASTNodeType* type;
	bool islvalue;
	if (lvaltype == ID) {
		int array_index;
		string array_id;
		bool isparam;
		islvalue = true;
		ret = find_id(gen_code_info, id, array_index, array_id, &type);
		check_type(gen_code_info, type);
		string type_asm = dynamic_cast<ASTNodeType*>
			(array_table[type->getValue()]->getChildren()[2])->getAsm();
		if (gen_code_info->params &&
			(gen_code_info->params->find(id) != gen_code_info->params->end())) {
			pre_result << "  %" << gen_code_info->tempval_count << " = load "
				<< type_asm << "** %" << array_index << ", align 4" << endl;
			array_index = gen_code_info->tempval_count++;
		}
		result << " = getelementptr inbounds " << type_asm;
		result << "* %";
		if (array_index >= 0)
			result << array_index;
		else
			result << array_id;
		result << ", i32 0";
	}
	else if (lvaltype == COMPOSED) {
		islvalue = gen_code_info->result.regval.islvalue;
		// intermediate arrays may not be stored in register
		type = gen_code_info->result.regval.type;
		check_type(gen_code_info, type);
		result << " = getelementptr inbounds " << dynamic_cast<ASTNodeType*>
			(array_table[type->getValue()]->getChildren()[2])->getAsm() << "* %"
			<< gen_code_info->result.regval.index << ", i32 0";
	}

	ret += dynamic_cast<ASTNodeExpression*>(children[1])->gen_code(gen_code_info);
	if (gen_code_info->result_type == GenCodeInfo::NONE ||
			gen_code_info->result_type == GenCodeInfo::FUNCTION) {
		ss << gen_code_info->loc << " error: array subscript is not an integer" << endl;
		throw runtime_error(ss.str());
	}
	else if (gen_code_info->result_type == GenCodeInfo::POINTER ||
			gen_code_info->result_type == GenCodeInfo::VALUE) {
		ASTNodeType* type = gen_code_info->result.regval.type; // shadows the type of array
		if (type->variableType() != ASTNodeType::INTEGER &&
				type->variableType() != ASTNodeType::BOOLEAN) {
			ss << gen_code_info->loc << " error: array subscript is not an integer" << endl;
			throw runtime_error(ss.str());
		}
		int subscript_index = gen_code_info->result.regval.index;
		string subscript_id = gen_code_info->result.regval.id;
		stringstream sss;
		if (gen_code_info->result_type == GenCodeInfo::POINTER) {
			sss << "  %" << gen_code_info->tempval_count << " = load " << type->getAsm() << "* %";
			if (subscript_index >= 0)
				sss << subscript_index;
			else
				sss << subscript_id;
			sss << ", align 4" << endl;
			subscript_index = gen_code_info->tempval_count++;
		}
		if (type->variableType() == ASTNodeType::BOOLEAN) {
			sss << "  %" << gen_code_info->tempval_count << " = zext i8 %"
				<< subscript_index << " to i32" << endl;
			subscript_index = gen_code_info->tempval_count++;
		}
		sss << "  %" << gen_code_info->tempval_count << result.str()
			<< ", i32 %" << subscript_index << endl;
		ret += sss.str();
	}
	else { // SIMPLE
		stringstream sss;
		ASTNodeExpression* expr = gen_code_info->result.expr;
		if (expr->type() == ASTNode::INTEGER || expr->type() == ASTNode::BOOLEAN) {
			int value;
			int length = array_table[type->getValue()]->getLength();
			if (expr->type() == ASTNode::INTEGER)
				value = dynamic_cast<ASTNodeInteger*>(expr)->getValue();
			else
				value = dynamic_cast<ASTNodeBoolean*>(expr)->getValue();
			if (value < 0 || value >= length) {
				ss << gen_code_info->loc << " error: array subscript out of range (expected 0 - "
					<< length - 1 << " but get " << value << " )" << endl;
				throw runtime_error(ss.str());
			}
			sss << "  %" << gen_code_info->tempval_count << result.str()
				<< ", i32 " << value << endl;
			ret += sss.str();
		}
		else if (expr->type() == ASTNode::STRING || expr->type() == ASTNode::THIS) {
			ss << gen_code_info->loc << " error: array subscript is not an integer" << endl;
			throw runtime_error(ss.str());
		}
		else if (expr->type() == ASTNode::IDENTIFIER) {
			int index;
			ASTNodeType* type; // shadows the type of array
			ret += load_id(gen_code_info, expr, index, NULL, &type);
			if (type->variableType() == ASTNodeType::INTEGER ||
					type->variableType() == ASTNodeType::BOOLEAN) {
				if (type->variableType() == ASTNodeType::BOOLEAN) {
					result << "  %" << gen_code_info->tempval_count
						<< " = zext i8 %" << index << " to i32" << endl;
					index = gen_code_info->tempval_count++;
				}
			}
			else {
				ss << gen_code_info->loc << " error: array subscript is not an integer" << endl;
				throw runtime_error(ss.str());
			}
			sss << "  %" << gen_code_info->tempval_count << result.str()
				<< ", i32 %" << index << endl;
			ret += sss.str();
		}
	}
	gen_code_info->result_type = GenCodeInfo::POINTER;
	gen_code_info->result.regval.index = gen_code_info->tempval_count++;
	type = dynamic_cast<ASTNodeType*>(array_table[type->getValue()]->getChildren()[2]);
	gen_code_info->result.regval.type = new ASTNodeType(type->getValue());
	children.push_back(gen_code_info->result.regval.type);
	gen_code_info->result.regval.islvalue = islvalue;
	gen_code_info->loc = loc;
	return pre_result.str() + ret;
}

string ASTNodeArrayAccess::gen_simple(GenCodeInfo* gen_code_info) {
	stringstream ss;
	ASTNodeExpression* expr = gen_code_info->result.expr;
	if (!expr) {
		throw runtime_error("panic: unexpected code path, BUG in code!\n");
	}
	if (expr->type() == ASTNode::IDENTIFIER) {
		return gen_asm(gen_code_info, ID);
	}
	else if (expr->type() == ASTNode::THIS) {
		if (gen_code_info->class_id.empty())
			ss << gen_code_info->loc << " error: invalid use of 'this' in non-member function" << endl;
		else
			ss << gen_code_info->loc << " error: can't use operator '[]' on 'this'" << endl;
		throw runtime_error(ss.str());
	}
	else if (expr->type() == ASTNode::INTEGER) {
		ss << gen_code_info->loc << " error: can't use operator '[]' on integer" << endl;
		throw runtime_error(ss.str());
	}
	else if (expr->type() == ASTNode::BOOLEAN) {
		ss << gen_code_info->loc << " error: can't use operator '[]' on boolean" << endl;
		throw runtime_error(ss.str());
	}
	else if (expr->type() == ASTNode::STRING) {
		ss << gen_code_info->loc << " error: can't use operator '[]' on string" << endl;
		throw runtime_error(ss.str());
	}
	else {
		ss << gen_code_info->loc << " panic: unexpected code path, BUG in code!" << endl;
		throw runtime_error(ss.str());
	}
}

string ASTNodeArrayAccess::gen_code(GenCodeInfo* gen_code_info) {
	stringstream ss;
	string ret = dynamic_cast<ASTNodeExpression*>(children[0])->gen_code(gen_code_info);
	if (gen_code_info->result_type == GenCodeInfo::NONE) {
		ss << gen_code_info->loc << " error: can't use operator '[]' on 'void' value" << endl;
		throw runtime_error(ss.str());
	}
	else if (gen_code_info->result_type == GenCodeInfo::SIMPLE) {
		return gen_simple(gen_code_info);
	}
	else if (gen_code_info->result_type == GenCodeInfo::FUNCTION) {
		ss << gen_code_info->loc << " error: can't use operator '[]' on function" << endl;
		throw runtime_error(ss.str());
	}
	else if (gen_code_info->result_type == GenCodeInfo::POINTER ||
			gen_code_info->result_type == GenCodeInfo::VALUE) {
		ASTNodeType* type = gen_code_info->result.regval.type;
		if (type->variableType() == ASTNodeType::INTEGER ||
				type->variableType() == ASTNodeType::BOOLEAN) {
			ss << gen_code_info->loc << " error: can't use operator '[]' on " << type->getValue() << endl;
			throw runtime_error(ss.str());
		}
		else if (class_table.find(type->getValue()) != class_table.end()) {
			ss << gen_code_info->loc << " error: can't use operator '[]' on class type '" << type->getValue() << "'" << endl;
			throw runtime_error(ss.str());
		}
		else if (array_table.find(type->getValue()) != array_table.end()) {
			if (gen_code_info->result_type == GenCodeInfo::VALUE) {
				ss << gen_code_info->loc << " panic: array type found in a register value, BUG in code!" << endl;
				throw runtime_error(ss.str());
			}
			return ret + gen_asm(gen_code_info, COMPOSED);
		}
		else {
			ss << gen_code_info->loc << " panic: unexpected code path, BUG in code!" << endl;
			throw runtime_error(ss.str());
		}
	}
	return ret;
}

string ASTNodeBinaryExpr::gen_assign(GenCodeInfo *gen_code_info) {
	stringstream ss;
	string code = dynamic_cast<ASTNodeExpression*>(children[0])->gen_code(gen_code_info);
	GenCodeInfo::ResultType left_result_type = gen_code_info->result_type;
	GenCodeInfo::Result left_result = gen_code_info->result;
	yy::location left_loc = gen_code_info->loc;
	if (left_result_type == GenCodeInfo::NONE || left_result_type == GenCodeInfo::FUNCTION ||
			(left_result_type == GenCodeInfo::SIMPLE && left_result.expr->type() == ASTNode::THIS)) {
		ss << left_loc << " error: expression is not assignable" << endl;
		throw runtime_error(ss.str());
	}
	else if ((left_result_type == GenCodeInfo::VALUE) ||
			(left_result_type == GenCodeInfo::POINTER && !left_result.regval.islvalue) ||
			(left_result_type == GenCodeInfo::SIMPLE &&
					(left_result.expr->type() == ASTNode::INTEGER ||
					 left_result.expr->type() == ASTNode::BOOLEAN ||
					 left_result.expr->type() == ASTNode::STRING))) {
		ss << left_loc << " error: lvalue required as left operand of assignment" << endl;
		throw runtime_error(ss.str());
	}
	else if (left_result_type == GenCodeInfo::SIMPLE) { // ID
		code += find_id(gen_code_info, dynamic_cast<ASTNodeID*>(left_result.expr)->getID(),
				left_result.regval.index, left_result.regval.id, &left_result.regval.type);
	}

	code += dynamic_cast<ASTNodeExpression*>(children[1])->gen_code(gen_code_info);
	GenCodeInfo::ResultType right_result_type = gen_code_info->result_type;
	GenCodeInfo::Result &right_result = gen_code_info->result;
	yy::location &right_loc = gen_code_info->loc;
	stringstream result;
	if (right_result_type == GenCodeInfo::NONE) {
		ss << right_loc << " error: cannot use 'void' type as right operand of assignment" << endl;
		throw runtime_error(ss.str());
	}
	else if (right_result_type == GenCodeInfo::FUNCTION) {
		ss << right_loc << " error: cannot use function as right operand of assignment" << endl;
		throw runtime_error(ss.str());
	}
	else if (right_result_type == GenCodeInfo::SIMPLE) {
		if (right_result.expr->type() == ASTNode::THIS) {
			ss << right_loc << " error: cannot use 'this' as right operand of assignment" << endl;
			throw runtime_error(ss.str());
		}
		else if (right_result.expr->type() == ASTNode::STRING) {
			ss << right_loc << " error: cannot use string as right operand of assignment" << endl;
			throw runtime_error(ss.str());
		}
		else if (right_result.expr->type() == ASTNode::INTEGER ||
				right_result.expr->type() == ASTNode::BOOLEAN) {
			// case 1: rvalue is literal
			if (left_result.regval.type->variableType() == ASTNodeType::INTEGER ||
					left_result.regval.type->variableType() == ASTNodeType::BOOLEAN) {
				int value;
				if (right_result.expr->type() == ASTNode::INTEGER)
					value = dynamic_cast<ASTNodeInteger*>(right_result.expr)->getValue();
				else
					value = dynamic_cast<ASTNodeBoolean*>(right_result.expr)->getValue();
				if (left_result.regval.type->variableType() == ASTNodeType::INTEGER)
					result << "  store i32 " << value << ", i32* %";
				else
					result << "  store i8 " << bool(value) << ", i8 * %";
				if (left_result.regval.index >= 0)
					result << left_result.regval.index;
				else
					result << left_result.regval.id;
				result << ", align 4" << endl;
				goto ret;
			}
			else {
				ss << loc << " error: assigning to '" << left_result.regval.type->getValue()
					<< "' from incompatible type '";
				if (right_result.expr->type() == ASTNode::INTEGER)
					ss << "integer";
				else
					ss << "boolean";
				ss << "'" << endl;
				throw runtime_error(ss.str());
			}
		}
		else { // ID
			code += find_id(gen_code_info, dynamic_cast<ASTNodeID*>(right_result.expr)->getID(),
					right_result.regval.index, right_result.regval.id, &right_result.regval.type);
		}
	}

	// case 2: rvalue is variable
	if (left_result.regval.type->getValue() != right_result.regval.type->getValue()) {
		if (!(	(left_result.regval.type->variableType() == ASTNodeType::INTEGER &&
					right_result.regval.type->variableType() == ASTNodeType::BOOLEAN) ||
				(left_result.regval.type->variableType() == ASTNodeType::BOOLEAN &&
					right_result.regval.type->variableType() == ASTNodeType::INTEGER)))
		{
			ss << loc << " error: assigning to '" << left_result.regval.type->getValue()
				<< "' from incompatible type '" << right_result.regval.type->getValue() << "'" << endl;
			throw runtime_error(ss.str());
		}
	}
	else if (array_table.find(left_result.regval.type->getValue()) != array_table.end()) {
		ss << left_loc << " error: cannot assign to an array" << endl;
		throw runtime_error(ss.str());
	}

	if (right_result_type == GenCodeInfo::SIMPLE || right_result_type == GenCodeInfo::POINTER) {
		result << "  %" << gen_code_info->tempval_count << " = load "
			<< right_result.regval.type->getTypeAsm(false) << "* %";
		if (right_result.regval.index >= 0)
			result << right_result.regval.index;
		else
			result << right_result.regval.id;
		result << ", align 4" << endl;
		right_result.regval.index = gen_code_info->tempval_count++;
	}
	if (left_result.regval.type->variableType() == ASTNodeType::INTEGER) {
		if (right_result.regval.type->variableType() == ASTNodeType::BOOLEAN) {
			result << "  %" << gen_code_info->tempval_count << " = zext i8 %"
				<< right_result.regval.index << " to i32" << endl;
			right_result.regval.index = gen_code_info->tempval_count++;
		}
		result << "  store i32 %" << right_result.regval.index << ", i32* %";
	}
	else if (left_result.regval.type->variableType() == ASTNodeType::BOOLEAN){
		if (right_result.regval.type->variableType() == ASTNodeType::INTEGER) {
			result << "  %" << gen_code_info->tempval_count++ << " = icmp ne i32 %"
				<< right_result.regval.index << ", 0" << endl;
			result << "  %" << gen_code_info->tempval_count << " = zext i1 %"
				<< gen_code_info->tempval_count - 1 << " to i8" << endl;
			right_result.regval.index = gen_code_info->tempval_count++;
		}
		result << "  store i8 %" << right_result.regval.index << ", i8* %";
	}
	else {
		result << "  store " << right_result.regval.type->getTypeAsm(false) << " %"
			<< right_result.regval.index << ", "
			<< right_result.regval.type->getTypeAsm(false) << "* %";
	}
	if (left_result.regval.index >= 0)
		result << left_result.regval.index;
	else
		result << left_result.regval.id;
	result << ", align 4" << endl;

ret:
	gen_code_info->result_type = GenCodeInfo::POINTER;
	gen_code_info->result = left_result;
	gen_code_info->result.regval.islvalue = true;
	gen_code_info->loc = loc;
	return code + result.str();
}

string ASTNodeBinaryExpr::gen_compute_load(GenCodeInfo *gen_code_info, bool &isconstant, bool &isbool, int &value, bool left) {
	stringstream ss, ret;
	GenCodeInfo::ResultType &result_type = gen_code_info->result_type;
	GenCodeInfo::Result &result = gen_code_info->result;
	yy::location &loc = gen_code_info->loc;
	if (result_type == GenCodeInfo::NONE) {
		ss << loc << " error: invalid operands to binary operator '"
			<< op << "' (" << (left ? "left" : "right") << " operand is 'void')" << endl;
		throw runtime_error(ss.str());
	}
	else if (result_type == GenCodeInfo::FUNCTION) {
		ss << loc << " error: invalid operands to binary operator '"
			<< op << "' (" << (left ? "left" : "right") << " operand is function)" << endl;
		throw runtime_error(ss.str());
	}
	else if ((result_type == GenCodeInfo::POINTER || result_type == GenCodeInfo::VALUE)) {
load_value:
		if (result.regval.type->variableType() == ASTNodeType::INTEGER ||
				result.regval.type->variableType() == ASTNodeType::BOOLEAN) {
			if (result_type == GenCodeInfo::POINTER) {
				ret << "  %" << gen_code_info->tempval_count << " = load "
					<< result.regval.type->getAsm() << "* %";
				if (result.regval.index >= 0)
					ret << result.regval.index;
				else
					ret << result.regval.id;
				ret << ", align 4" << endl;
				result.regval.index = gen_code_info->tempval_count++;
			}
		}
		else {
			string type_name = result.regval.type->getValue();
			if (array_table.find(type_name) != array_table.end())
				type_name = "array type '" + type_name + "'";
			else if (class_table.find(type_name) != class_table.end())
				type_name = "class type '" + type_name + "'";
			ss << loc << " error: invalid operands to binary operator '" << op
				<< "' (" << (left ? "left" : "right") << " operand is " << type_name << ")" << endl;
			throw runtime_error(ss.str());
		}
	}
	else if(result_type == GenCodeInfo::SIMPLE) {
		if (result.expr->type() == ASTNode::THIS) {
			ss << loc << " error: invalid operands to binary operator '"
				<< op << "' (" << (left ? "left" : "right") << " operand is 'this')" << endl;
			throw runtime_error(ss.str());
		}
		else if (result.expr->type() == ASTNode::STRING) {
			ss << loc << " error: invalid operands to binary operator '"
				<< op << "' (" << (left ? "left" : "right") << " operand is 'string')" << endl;
			throw runtime_error(ss.str());
		}
		else if (result.expr->type() == ASTNode::INTEGER) {
			isconstant = true;
			isbool = false;
			value = dynamic_cast<ASTNodeInteger*>(result.expr)->getValue();
		}
		else if (result.expr->type() == ASTNode::BOOLEAN) {
			isconstant = true;
			isbool = true;
			value = dynamic_cast<ASTNodeBoolean*>(result.expr)->getValue();
		}
		else {
			ret << find_id(gen_code_info, dynamic_cast<ASTNodeID*>(result.expr)->getID(),
					result.regval.index, result.regval.id, &result.regval.type);
			result_type = GenCodeInfo::POINTER;
			goto load_value;
		}
	}
	return ret.str();
}

string ASTNodeBinaryExpr::gen_compute(GenCodeInfo *gen_code_info) {
	stringstream ss, result;
	string lcode = dynamic_cast<ASTNodeExpression*>(children[0])->gen_code(gen_code_info);
	bool left_isconstant = false;
	bool left_isbool;
	int left_value;
	lcode += gen_compute_load(gen_code_info, left_isconstant, left_isbool, left_value, true);
	GenCodeInfo::Result left_result = gen_code_info->result;
	
	int left_block, right_block, left_tempval;
	if ((op == "or" || op == "and") && !left_isconstant) {
		left_block = gen_code_info->current_block;
		left_tempval = gen_code_info->tempval_count;
		gen_code_info->tempval_count += 2;
		right_block = gen_code_info->current_block = left_tempval + 1;
	}

	string rcode = dynamic_cast<ASTNodeExpression*>(children[1])->gen_code(gen_code_info);
	bool right_isconstant = false;
	bool right_isbool;
	int right_value;

	rcode += gen_compute_load(gen_code_info, right_isconstant, right_isbool, right_value, false);
	// Note this is a reference, we will set gen_code_info->result through right_result.
	GenCodeInfo::Result &right_result = gen_code_info->result;

	if (op == "or" || op == "and") {
		if (left_isconstant) {
			if (op == "or" && left_value != 0) {
				// true || EXPR
				children.push_back(new ASTNodeBoolean(true));
				goto ret_constant;
			}
			else if (op == "and" && left_value == 0) {
				// false && EXPR
				children.push_back(new ASTNodeBoolean(false));
				goto ret_constant;
			}
			else {
				// false || EXPR or true && EXPR
				if (right_isconstant) {
					children.push_back(new ASTNodeBoolean(right_value != 0));
					goto ret_constant;
				}
				else {
					if (right_result.regval.type->variableType() == ASTNodeType::INTEGER) {
						result << "  %" << gen_code_info->tempval_count++ << " = icmp ne i32 %"
							<< right_result.regval.index << ", 0" << endl;
						result << "  %" << gen_code_info->tempval_count << " = zext i1 %"
							<< gen_code_info->tempval_count - 1 << " to i8" << endl;
						right_result.regval.index = gen_code_info->tempval_count++;
					}
					right_result.regval.type = new ASTNodeType(ASTNodeType::BOOLEAN);
					children.push_back(right_result.regval.type);
					goto ret_regval;
				}
			}
		}
		else {
			stringstream lresult;
			int end_block;
			if (!right_isconstant) {
				result << "  %" << gen_code_info->tempval_count++ << " = icmp ne "
					<< right_result.regval.type->getAsm() << " %"
					<< right_result.regval.index << ", 0" << endl;
			}
			result << "  br label %" << gen_code_info->tempval_count << endl;
			end_block = gen_code_info->tempval_count++;
			result << endl;
			result << "; <label>:";
			result.setf(ios::left, ios::adjustfield);
			result.width(40);
			result << end_block << "; preds = %" << gen_code_info->current_block
				<< ", %" << left_block << endl;
			result.unsetf(ios::adjustfield);
			result << "  %" << gen_code_info->tempval_count++ << " = phi i1 [ ";
			if (op == "or")
				result << "true, %" << left_block << " ], [ ";
			else
				result << "false, %" << left_block << " ], [ ";
			if (right_isconstant) {
				if (right_value != 0)
					result << "true";
				else
					result << "false";
			}
			else
				result << "%" << end_block - 1;
			result << ", %" << gen_code_info->current_block << " ]" << endl;
			result << "  %" << gen_code_info->tempval_count << " = zext i1 %"
				<< gen_code_info->tempval_count - 1 << " to i8" << endl;

			lresult << "  %" << left_tempval << " = icmp ne " << left_result.regval.type->getAsm()
				<< " %" << left_result.regval.index << ", 0" << endl;
			lresult << "  br i1 %" << left_tempval << ", label %";
			if (op == "or")
				lresult << end_block << ", label %" << right_block << endl;
			else
				lresult << right_block << ", label %" << end_block << endl;
			lresult << endl;
			lresult << "; <label>:";
			lresult.setf(ios::left, ios::adjustfield);
			lresult.width(40);
			lresult << right_block << "; preds = %" << left_block << endl;
			lresult.unsetf(ios::adjustfield);
			lcode += lresult.str();

			gen_code_info->current_block = end_block;
			right_result.regval.index = gen_code_info->tempval_count++;
			right_result.regval.type = new ASTNodeType(ASTNodeType::BOOLEAN);
			children.push_back(right_result.regval.type);
			goto ret_regval;
		}
	}

	else {
		if (left_isconstant && right_isconstant) {
			if (op == "|")
				children.push_back(new ASTNodeInteger(left_value | right_value));
			else if (op == "^")
				children.push_back(new ASTNodeInteger(left_value ^ right_value));
			else if (op == "&")
				children.push_back(new ASTNodeInteger(left_value & right_value));
			else if (op == "<<")
				children.push_back(new ASTNodeInteger(left_value << right_value));
			else if (op == ">>")
				children.push_back(new ASTNodeInteger(left_value >> right_value));
			else if (op == "+")
				children.push_back(new ASTNodeInteger(left_value + right_value));
			else if (op == "-")
				children.push_back(new ASTNodeInteger(left_value - right_value));
			else if (op == "*")
				children.push_back(new ASTNodeInteger(left_value * right_value));
			else if (op == "/" || op == "%") {
				if (right_value == 0) {
					ss << gen_code_info->loc << " error: divide by zero" << endl;
					throw runtime_error(ss.str());
				}
				if (op == "/")
					children.push_back(new ASTNodeInteger(left_value / right_value));
				else
					children.push_back(new ASTNodeInteger(left_value % right_value));
			}

			else if (op == "==")
				children.push_back(new ASTNodeBoolean(bool(left_value == right_value)));
			else if (op == "!=")
				children.push_back(new ASTNodeBoolean(bool(left_value != right_value)));
			else if (op == "<=")
				children.push_back(new ASTNodeBoolean(bool(left_value <= right_value)));
			else if (op == ">=")
				children.push_back(new ASTNodeBoolean(bool(left_value >= right_value)));
			else if (op == "<")
				children.push_back(new ASTNodeBoolean(bool(left_value < right_value)));
			else if (op == ">")
				children.push_back(new ASTNodeBoolean(bool(left_value > right_value)));
			goto ret_constant;
		}
		if (!left_isconstant && left_result.regval.type->variableType() == ASTNodeType::BOOLEAN) {
			result << "  %" << gen_code_info->tempval_count << " = zext i8 %"
				<< left_result.regval.index << " to i32" << endl;
			left_result.regval.index = gen_code_info->tempval_count++;
		}
		if (!right_isconstant && right_result.regval.type->variableType() == ASTNodeType::BOOLEAN) {
			result << "  %" << gen_code_info->tempval_count << " = zext i8 %"
				<< right_result.regval.index << " to i32" << endl;
			right_result.regval.index = gen_code_info->tempval_count++;
		}

		result << "  %" << gen_code_info->tempval_count;
		if (op == "|")
			result << " = or i32 ";
		else if (op == "^")
			result << " = xor i32 ";
		else if (op == "&")
			result << " = and i32 ";
		else if (op == "<<")
			result << " = shl i32 ";
		else if (op == ">>")
			result << " = ashr i32 ";
		else if (op == "+")
			result << " = add nsw i32 ";
		else if (op == "-")
			result << " = sub nsw i32 ";
		else if (op == "*")
			result << " = mul nsw i32 ";
		else if (op == "/")
			result << " = sdiv i32 ";
		else if (op == "%")
			result << " = srem i32 ";
		else if (op == "==")
			result << " = icmp eq i32 ";
		else if (op == "!=")
			result << " = icmp ne i32 ";
		else if (op == "<=")
			result << " = icmp sle i32 ";
		else if (op == ">=")
			result << " = icmp sge i32 ";
		else if (op == "<")
			result << " = icmp slt i32 ";
		else if (op == ">")
			result << " = icmp sgt i32 ";
		if (left_isconstant)
			result << left_value;
		else
			result << "%" << left_result.regval.index;
		result << ", ";
		if (right_isconstant)
			result << right_value;
		else
			result << "%" << right_result.regval.index;
		result << endl;
		if (op == "==" || op == "!=" || op == "<=" || op == ">=" || op == "<" || op == ">") {
			result << "  %" << gen_code_info->tempval_count + 1 << " = zext i1 %"
				<< gen_code_info->tempval_count << " to i8" << endl;
			gen_code_info->tempval_count++;
			right_result.regval.type = new ASTNodeType(ASTNodeType::BOOLEAN);
		}
		else
			right_result.regval.type = new ASTNodeType(ASTNodeType::INTEGER);
		children.push_back(right_result.regval.type);
		right_result.regval.index = gen_code_info->tempval_count++;
		goto ret_regval;
	}

ret_regval:
	gen_code_info->result_type = GenCodeInfo::VALUE;
	gen_code_info->result.regval.islvalue = false;
	gen_code_info->loc = loc;
	return lcode + rcode + result.str();

ret_constant:
	gen_code_info->result_type = GenCodeInfo::SIMPLE;
	gen_code_info->result.expr = dynamic_cast<ASTNodeExpression*>(children[2]);
	gen_code_info->loc = loc;
	return lcode;
}

string ASTNodeBinaryExpr::gen_code(GenCodeInfo* gen_code_info) {
	if (op == ":=")
		return gen_assign(gen_code_info);
	else
		return gen_compute(gen_code_info);
}

string ASTNodeMethodInvocation::gen_code(GenCodeInfo* gen_code_info) {
	stringstream ss;
	string code;
	bool isglobal = false;
	GenCodeInfo::Result func_result;
	string func_name;
	map<string, pair<int, ASTNodeType*>>* params;
	ASTNodeType* return_type;
	stringstream pre_call;
	if (children[0]->type() == ASTNode::IDENTIFIER) {
		func_name = dynamic_cast<ASTNodeID*>(children[0])->getID();
		if (g_func_table.find(func_name) == g_func_table.end()) {
			if (gen_code_info->func_table &&
					(gen_code_info->func_table->find(func_name) != gen_code_info->func_table->end())) {
				pre_call << "  %" << gen_code_info->tempval_count << " = load %class."
					<< gen_code_info->class_id << "** %1, align 4" << endl;
				code = pre_call.str();
				func_result.func.this_index = gen_code_info->tempval_count++;
				func_result.func.class_id = gen_code_info->class_id;
				func_result.func.func = (*gen_code_info->func_table)[func_name];
				params = func_result.func.func->getParams();
				return_type = dynamic_cast<ASTNodeType*>(func_result.func.func->getChildren()[3]);
			}
			else {
				ss << children[0]->getLoc() << " error: function '" << func_name
					<< "' is not declared" << endl;
				throw runtime_error(ss.str());
			}
		}
		else {
			isglobal = true;
			params = g_func_table[func_name]->getParams();
			return_type = dynamic_cast<ASTNodeType*>(g_func_table[func_name]->getChildren()[3]);
		}
	}
	else {
		code = dynamic_cast<ASTNodeExpression*>(children[0])->gen_code(gen_code_info);
		if (gen_code_info->result_type != GenCodeInfo::FUNCTION) {
			ss << gen_code_info->loc << " error: called object is not a function" << endl;
			throw runtime_error(ss.str());
		}
		func_result = gen_code_info->result;
		func_name = dynamic_cast<ASTNodeID*>(func_result.func.func->getChildren()[0])->getID();
		params = func_result.func.func->getParams();
		return_type = dynamic_cast<ASTNodeType*>(func_result.func.func->getChildren()[3]);
	}

	stringstream call;
	call << "call " << return_type->getTypeAsm(false) << " @";
	if (isglobal) {
		if (func_name == "main")
			call << "...main(";
		else
			call << func_name << "(";
	}
	else {
		call << "class." << func_result.func.class_id << "." << func_name << "(%class."
			<< func_result.func.class_id << "* %";
		if (func_result.func.this_index >= 0)
			call << func_result.func.this_index;
		else
			call << func_result.func.this_id;
		if (!params->empty())
			call << ", ";
	}
	
	vector<ASTNode*>& param_list = children[1]->getChildren();
	if (param_list.size() != params->size()) {
		ss << loc << " error: function requires " << params->size() << " arguments, but "
			<< param_list.size() << " was provided" << endl;
		throw runtime_error(ss.str());
	}
	vector<ASTNodeType*> param_type(params->size());
	for (auto iter : *params)
		param_type[iter.second.first] = iter.second.second;

	for (int i = 0; i < param_list.size(); ++i) {
		stringstream code_add;
		if (i != 0)
			call << ", ";
		ASTNodeType *type = param_type[i];
		code += dynamic_cast<ASTNodeExpression*>(param_list[i])->gen_code(gen_code_info);
		GenCodeInfo::Result &result = gen_code_info->result;
		if (gen_code_info->result_type == GenCodeInfo::NONE) {
			ss << gen_code_info->loc << " error: invalid argument type 'void'" << endl;
			throw runtime_error(ss.str());
		}
		else if (gen_code_info->result_type == GenCodeInfo::FUNCTION) {
			ss << gen_code_info->loc << " error: cannot pass function as an argument" << endl;
			throw runtime_error(ss.str());
		}
		else if (gen_code_info->result_type == GenCodeInfo::POINTER ||
				gen_code_info->result_type == GenCodeInfo::VALUE) {
			if (gen_code_info->result_type == GenCodeInfo::POINTER &&
					array_table.find(result.regval.type->getValue()) == array_table.end()) {
				code_add << "  %" << gen_code_info->tempval_count << " = load "
					<< result.regval.type->getTypeAsm(false) << "* %";
				if (result.regval.index >= 0)
					code_add << result.regval.index;
				else
					code_add << result.regval.id;
				code_add << ", align 4" << endl;
				result.regval.index = gen_code_info->tempval_count++;
			}
call_variable:
			if (result.regval.type->getValue() != type->getValue()) {
				if (result.regval.type->variableType() == ASTNodeType::INTEGER &&
						type->variableType() == ASTNodeType::BOOLEAN) {
					code_add << "  %" << gen_code_info->tempval_count++ << " = icmp ne i32 %"
						<< result.regval.index << ", 0" << endl;
					code_add << "  %" << gen_code_info->tempval_count << " = zext i1 %"
						<< gen_code_info->tempval_count - 1 << " to i8" << endl;
					result.regval.index = gen_code_info->tempval_count++;
				}
				else if (result.regval.type->variableType() == ASTNodeType::BOOLEAN &&
						type->variableType() == ASTNodeType::INTEGER) {
					code_add << "  %" << gen_code_info->tempval_count << " = zext i8 %"
						<< result.regval.index << " to i32" << endl;
					result.regval.index = gen_code_info->tempval_count++;
				}
				else {
					ss << gen_code_info->loc << " error: argument type not match, expected type '"
						<< type->getValue() << "'" << endl;
					throw runtime_error(ss.str());
				}
			}
			code += code_add.str();
			call << type->getTypeAsm(true) << " %";
			if (result.regval.index >= 0)
				call << result.regval.index;
			else
				call << result.regval.id;
		}
		else {
			ASTNodeExpression *expr = result.expr;
			if (expr->type() == ASTNode::THIS) {
				ss << gen_code_info->loc << " error: cannot pass 'this' as an argument" << endl;
				ss << "(since we can only pass class by value and 'this' is a pointer)" << endl;
				throw runtime_error(ss.str());
			}
			else if (expr->type() == ASTNode::STRING) {
				ss << gen_code_info->loc << " error: cannot pass string as an argument" << endl;
				throw runtime_error(ss.str());
			}
			else if (expr->type() == ASTNode::INTEGER || expr->type() == ASTNode::BOOLEAN) {
				int value;
				if (expr->type() == ASTNode::INTEGER)
					value = dynamic_cast<ASTNodeInteger*>(expr)->getValue();
				else
					value = dynamic_cast<ASTNodeBoolean*>(expr)->getValue();
				if (type->variableType() == ASTNodeType::INTEGER)
					call << "i32 " << value;
				else if (type->variableType() == ASTNodeType::BOOLEAN)
					call << "i8 " << bool(value);
				else {
					ss << gen_code_info->loc << " error: argument type not match, expected type '"
						<< type->getValue() << "'" << endl;
					throw runtime_error(ss.str());
				}
			}
			else { //ID
				code += load_id(gen_code_info, expr, result.regval.index, &result.regval.id, &result.regval.type);
				goto call_variable;
			}
		}
	}
	call << ")" << endl;

	if (return_type->variableType() == ASTNodeType::VOID) {
		code += "  " + call.str();
		gen_code_info->result_type = GenCodeInfo::NONE;
	}
	else {
		stringstream sss;
		sss << "  %" << gen_code_info->tempval_count << " = ";
		code += sss.str() + call.str();
		gen_code_info->result_type = GenCodeInfo::VALUE;
		gen_code_info->result.regval.index = gen_code_info->tempval_count++;
		gen_code_info->result.regval.type = return_type;
	}

	gen_code_info->result.regval.islvalue = false;
	gen_code_info->loc = loc;
	return code;
}

//-----------------------------Statements---------------------------------

string ASTNodePrintStmt::gen_simple(GenCodeInfo* gen_code_info) {
	stringstream ss, result;
	ASTNodeExpression* expr = gen_code_info->result.expr;
	if (!expr) {
		throw runtime_error("panic: unexpected code path, BUG in code!\n");
	}
	if (expr->type() == ASTNode::IDENTIFIER) {
		int index;
		ASTNodeType *type;
		result << load_id(gen_code_info, expr, index, NULL, &type);
		if (type->variableType() == ASTNodeType::INTEGER ||
				type->variableType() == ASTNodeType::BOOLEAN) {
			if (type->variableType() == ASTNodeType::BOOLEAN) {
				result << "  %" << gen_code_info->tempval_count << " = zext i8 %"
					<< index << " to i32" << endl;
				index = gen_code_info->tempval_count++;
			}
			result << "  %" << gen_code_info->tempval_count++
				<< " = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds (["
				<< 3 <<" x i8]* @.str" << str_table["%d"] <<", i32 0, i32 0), i32 %"
				<< index << ")" << endl;
		}
		else if (array_table.find(type->getValue()) != array_table.end()) {
			ss << gen_code_info->loc << " error[TODO]: we don't support printing array" << endl;
			throw runtime_error(ss.str());
		}
		else if (class_table.find(type->getValue()) != class_table.end()) {
			ss << gen_code_info->loc << " error[TODO]: we don't support printing class" << endl;
			throw runtime_error(ss.str());
		}
		else {
			ss << gen_code_info->loc << " panic: unexpected code path, BUG in code!" << endl;
			throw runtime_error(ss.str());
		}
	}
	else if (expr->type() == ASTNode::THIS) {
		if (gen_code_info->class_id.empty())
			ss << gen_code_info->loc << " error: invalid use of 'this' in non-member function" << endl;
		else
			ss << gen_code_info->loc << " error[TODO]: we don't support printing class" << endl;
		throw runtime_error(ss.str());
	}
	else if (expr->type() == ASTNode::INTEGER || expr->type() == ASTNode::BOOLEAN) {
		int value;
		if (expr->type() == ASTNode::INTEGER)
			value = dynamic_cast<ASTNodeInteger*>(expr)->getValue();
		else
			value = dynamic_cast<ASTNodeBoolean*>(expr)->getValue();
		result << "  %" << gen_code_info->tempval_count++
			<< " = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds (["
			<< 3 <<" x i8]* @.str" << str_table["%d"] <<", i32 0, i32 0), i32 "
			<< value << ")" << endl;
	}
	else if (expr->type() == ASTNode::STRING) {
		string str = dynamic_cast<ASTNodeString*>(expr)->getValue();
		result << "  %" << gen_code_info->tempval_count++
			<< " = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds (["
			<< str.size() + 1 <<" x i8]* @.str" << str_table[str] <<", i32 0, i32 0))" << endl;
	}
	else {
		ss << gen_code_info->loc << " panic: unexpected code path, BUG in code!" << endl;
		throw runtime_error(ss.str());
	}
	return result.str();
}

string ASTNodePrintStmt::gen_composed(GenCodeInfo* gen_code_info) {
	stringstream ss, result;
	int index = gen_code_info->result.regval.index;
	string id = gen_code_info->result.regval.id;
	if (gen_code_info->result_type == GenCodeInfo::POINTER) {
		result << "  %" << gen_code_info->tempval_count << " = load "
			<< gen_code_info->result.regval.type->getAsm() << "* %";
		if (index >= 0)
			result << index;
		else
			result << id;
		result << ", align 4" << endl;
		index = gen_code_info->tempval_count++;
	}
	if (gen_code_info->result.regval.type->variableType() == ASTNodeType::BOOLEAN) {
		result << "  %" << gen_code_info->tempval_count << " = zext i8 %"
			<< index << " to i32" << endl;
		index = gen_code_info->tempval_count++;
	}
	result << "  %" << gen_code_info->tempval_count++
		<< " = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds (["
		<< 3 <<" x i8]* @.str" << str_table["%d"] <<", i32 0, i32 0), i32 %"
		<< index << ")" << endl;
	return result.str();
}

string ASTNodePrintStmt::gen_code(GenCodeInfo* gen_code_info, ASTNodeExpression* expr) {
	stringstream ss;
	string ret = expr->gen_code(gen_code_info);
	if (gen_code_info->result_type == GenCodeInfo::NONE) {
		ss << gen_code_info->loc << " error: can't print void value" << endl;
		throw runtime_error(ss.str());
	}
	else if (gen_code_info->result_type == GenCodeInfo::SIMPLE) {
		return gen_simple(gen_code_info);
	}
	else if (gen_code_info->result_type == GenCodeInfo::FUNCTION) {
		ss << gen_code_info->loc << " error: can't print a function" << endl;
		throw runtime_error(ss.str());
	}
	else if (gen_code_info->result_type == GenCodeInfo::POINTER ||
			gen_code_info->result_type == GenCodeInfo::VALUE) {
		ASTNodeType* type = gen_code_info->result.regval.type;
		if (type->variableType() == ASTNodeType::INTEGER ||
				type->variableType() == ASTNodeType::BOOLEAN) {
			return ret + gen_composed(gen_code_info);
		}
		else if (class_table.find(type->getValue()) != class_table.end()) {
			ss << gen_code_info->loc << " error[TODO]: we don't support printing class" << endl;
			throw runtime_error(ss.str());
		}
		else if (array_table.find(type->getValue()) != array_table.end()) {
			ss << gen_code_info->loc << " error[TODO]: we don't support printing array" << endl;
			throw runtime_error(ss.str());
		}
		else {
			ss << gen_code_info->loc << " panic: unexpected code path, BUG in code!" << endl;
			throw runtime_error(ss.str());
		}
	}
}

string ASTNodePrintStmt::gen_code(GenCodeInfo* gen_code_info) {
	stringstream ss;
	vector<ASTNode*> &expr_list = children[0]->getChildren();
	if (expr_list.empty()) {
		ss << loc << " error: print statement may not be empty" << endl;
		throw runtime_error(ss.str());
	}
	for (int i = 0; i < expr_list.size(); ++i)
		ss << gen_code(gen_code_info, dynamic_cast<ASTNodeExpression*>(expr_list[i]));
	return ss.str();
}

string ASTNodeReturnStmt::gen_code(GenCodeInfo* gen_code_info) {
	stringstream ss, result;
	string code;
	GenCodeInfo::Result &ret_result = gen_code_info->result;
	ASTNodeType *ret_type = gen_code_info->ret_type;
	if (children.empty()) {
		if (gen_code_info->ret_type->variableType() != ASTNodeType::VOID) {
			ss << loc << " error: non-void function should return a value" << endl;
			throw runtime_error(ss.str());
		}
		else
			code = "  ret void\n";
	}
	else {
		code = dynamic_cast<ASTNodeExpression*>(children[0])->gen_code(gen_code_info);
		if (gen_code_info->result_type == GenCodeInfo::NONE) {
			ss << gen_code_info->loc << " error: cannot return value of incomplete type 'void'" << endl;
			throw runtime_error(ss.str());
		}
		else if (gen_code_info->result_type == GenCodeInfo::FUNCTION) {
			ss << gen_code_info->loc << " error: cannot return a function" << endl;
			throw runtime_error(ss.str());
		}
		// since we can't return array type, it's safe not to take it into consideration
		else if (gen_code_info->result_type == GenCodeInfo::POINTER ||
				gen_code_info->result_type == GenCodeInfo::VALUE) {
			if (gen_code_info->result_type == GenCodeInfo::POINTER) {
				result << "  %" << gen_code_info->tempval_count << " = load "
					<< ret_result.regval.type->getTypeAsm(false) << "* %";
				if (ret_result.regval.index >= 0)
					result << ret_result.regval.index;
				else
					result << ret_result.regval.id;
				result << ", align 4" << endl;
				ret_result.regval.index = gen_code_info->tempval_count++;
			}
ret_variable:
			if (ret_result.regval.type->getValue() != ret_type->getValue()) {
				if (ret_result.regval.type->variableType() == ASTNodeType::INTEGER &&
						ret_type->variableType() == ASTNodeType::BOOLEAN) {
					result << "  %" << gen_code_info->tempval_count++ << " = icmp ne i32 %"
						<< ret_result.regval.index << ", 0" << endl;
					result << "  %" << gen_code_info->tempval_count << " = zext i1 %"
						<< gen_code_info->tempval_count - 1 << " to i8" << endl;
					ret_result.regval.index = gen_code_info->tempval_count++;
				}
				else if (ret_result.regval.type->variableType() == ASTNodeType::BOOLEAN &&
						ret_type->variableType() == ASTNodeType::INTEGER) {
					result << "  %" << gen_code_info->tempval_count << " = zext i8 %"
						<< ret_result.regval.index << " to i32" << endl;
					ret_result.regval.index = gen_code_info->tempval_count++;
				}
				else {
					ss << gen_code_info->loc << " error: return value type not match, expected type '"
						<< ret_type->getValue() << "'" << endl;
					throw runtime_error(ss.str());
				}
			}
			result << "  ret " << ret_type->getTypeAsm(false) << " %" << ret_result.regval.index << endl;
			code += result.str();
		}
		else {
			ASTNodeExpression* expr = gen_code_info->result.expr;
			if (expr->type() == ASTNode::THIS) {
				ss << gen_code_info->loc << " error: cannot return 'this'" << endl;
				ss << "(since it's a pointer and we don't provide"
					<< "general pointer operation in MyLang)" << endl;
				throw runtime_error(ss.str());
			}
			else if (expr->type() == ASTNode::STRING) {
				ss << gen_code_info->loc << " error: cannot return a string" << endl;
				throw runtime_error(ss.str());
			}
			else if (expr->type() == ASTNode::INTEGER || expr->type() == ASTNode::BOOLEAN) {
				int value;
				if (expr->type() == ASTNode::INTEGER)
					value = dynamic_cast<ASTNodeInteger*>(expr)->getValue();
				else
					value = dynamic_cast<ASTNodeBoolean*>(expr)->getValue();
				if (ret_type->variableType() == ASTNodeType::INTEGER) {
					result << "  ret i32 " << value << endl;
					code += result.str();
				}
				else if (ret_type->variableType() == ASTNodeType::BOOLEAN) {
					result << "  ret i8 " << value << endl;
					code += result.str();
				}
				else {
					ss << gen_code_info->loc << " error: return value type not match, expected type'"
						<< ret_type->getValue() << "'" << endl;
					throw runtime_error(ss.str());
				}
			}
			else { //ID
				code += load_id(gen_code_info, expr, ret_result.regval.index, NULL, &ret_result.regval.type);
				goto ret_variable;
			}
		}
	}
	gen_code_info->block_isover = true;
	gen_code_info->terminated_bybr = false;
	return code;
}

string ASTNodeBlock::gen_code(GenCodeInfo* gen_code_info) {
	stringstream ss;
	int i;
	gen_code_info->block_isover = false;
	for (i = 0; i < children.size(); ++i) {
		ss << dynamic_cast<ASTNodeStatement*>(children[i])->gen_code(gen_code_info);
		if (gen_code_info->block_isover)
			break;
	}
	if (i < children.size()) {
		// gen code for following statements to check their correctness
		// and don't output them
		bool terminated_bybr = gen_code_info->terminated_bybr;
		vector<int> break_point = gen_code_info->break_point;
		vector<int> continue_point = gen_code_info->continue_point;
		int tempval_count = gen_code_info->tempval_count;
		int current_block = gen_code_info->current_block;
		for (; i < children.size(); ++i)
			dynamic_cast<ASTNodeStatement*>(children[i])->gen_code(gen_code_info);
		gen_code_info->block_isover = true;
		gen_code_info->terminated_bybr = terminated_bybr;
		gen_code_info->break_point = break_point;
		gen_code_info->continue_point = continue_point;
		gen_code_info->tempval_count = tempval_count;
		gen_code_info->current_block = current_block;
	}
	return ss.str();
}

string ASTNodeBreakStmt::gen_code(GenCodeInfo* gen_code_info) {
	stringstream ss;
	if (gen_code_info->in_loop) {
		ss << "  br label %" << BREAK_FLAG << string(HOLE_WIDTH - 1, ' ') << endl;
		gen_code_info->block_isover = true;
		gen_code_info->terminated_bybr = true;
		gen_code_info->break_point.push_back(gen_code_info->current_block);
		return ss.str();
	}
	else {
		ss << loc << " error: 'break' statement not in loop statement" << endl;
		throw runtime_error(ss.str());
	}
}

string ASTNodeContinueStmt::gen_code(GenCodeInfo* gen_code_info) {
	stringstream ss;
	if (gen_code_info->in_loop) {
		ss << "  br label %" << CONTINUE_FLAG << string(HOLE_WIDTH - 1, ' ') << endl;
		gen_code_info->block_isover = true;
		gen_code_info->terminated_bybr = true;
		gen_code_info->continue_point.push_back(gen_code_info->current_block);
		return ss.str();
	}
	else {
		ss << loc << " error: 'continue' statement not in loop statement" << endl;
		throw runtime_error(ss.str());
	}
}

static string load_bool(GenCodeInfo* gen_code_info, ASTNodeExpression* expr,
		int &index, bool &isconstant, bool &value) {
	stringstream ss, result;
	string code;
	GenCodeInfo::Result &expr_result = gen_code_info->result;
	code = expr->gen_code(gen_code_info);
	if (gen_code_info->result_type == GenCodeInfo::NONE) {
		ss << gen_code_info->loc << " error: expected boolean value, but get value of type 'void'" << endl;
		throw runtime_error(ss.str());
	}
	else if (gen_code_info->result_type == GenCodeInfo::FUNCTION) {
		ss << gen_code_info->loc << " error: expected boolean value, but get a function" << endl;
		throw runtime_error(ss.str());
	}
	else if (gen_code_info->result_type == GenCodeInfo::POINTER ||
			gen_code_info->result_type == GenCodeInfo::VALUE) {
		if (gen_code_info->result_type == GenCodeInfo::POINTER) {
			result << "  %" << gen_code_info->tempval_count << " = load "
				<< expr_result.regval.type->getAsm() << "* %";
			if (expr_result.regval.index >= 0)
				result << expr_result.regval.index;
			else
				result << expr_result.regval.id;
			result << ", align 4" << endl;
			expr_result.regval.index = gen_code_info->tempval_count++;
		}
load_variable:
		if (expr_result.regval.type->variableType() == ASTNodeType::INTEGER) {
			result << "  %" << gen_code_info->tempval_count << " = icmp ne i32 %"
				<< expr_result.regval.index << ", 0" << endl;
			expr_result.regval.index = gen_code_info->tempval_count++;
		}
		else if (expr_result.regval.type->variableType() == ASTNodeType::BOOLEAN) {
			result << "  %" << gen_code_info->tempval_count << " = trunc i8 %"
				<< expr_result.regval.index << " to i1" << endl;
			expr_result.regval.index = gen_code_info->tempval_count++;
		}
		else {
			ss << gen_code_info->loc << " error: expected boolean type, but get type '"
				<< expr_result.regval.type->getValue() << "'" << endl;
			throw runtime_error(ss.str());
		}
		isconstant = false;
		index = expr_result.regval.index;
		code += result.str();
	}
	else {
		ASTNodeExpression* expr = gen_code_info->result.expr;
		if (expr->type() == ASTNode::THIS) {
			ss << gen_code_info->loc << " error: expected boolean value, but get 'this'" << endl;
			throw runtime_error(ss.str());
		}
		else if (expr->type() == ASTNode::STRING) {
			ss << gen_code_info->loc << " error: expected boolean value, but get a string" << endl;
			throw runtime_error(ss.str());
		}
		else if (expr->type() == ASTNode::INTEGER || expr->type() == ASTNode::BOOLEAN) {
			int val;
			if (expr->type() == ASTNode::INTEGER)
				val = dynamic_cast<ASTNodeInteger*>(expr)->getValue();
			else
				val = dynamic_cast<ASTNodeBoolean*>(expr)->getValue();
			isconstant = true;
			value = bool(val);
		}
		else { //ID
			code += load_id(gen_code_info, expr, expr_result.regval.index, NULL, &expr_result.regval.type);
			goto load_variable;
		}
	}
	return code;
}

static void replace_label(string &block, string break_label, string continue_label) {
	for (int i = 0; i < block.size(); ++i) {
		if (block[i] == BREAK_FLAG) {
			if (break_label.size() <= HOLE_WIDTH) {
				for (int j = 0; j < break_label.size(); ++j)
					block[i + j] = break_label[j];
				i += HOLE_WIDTH - 1;
			}
			else {
				for (int j = 0; j < HOLE_WIDTH; ++j)
					block[i + j] = break_label[j];
				block.insert(i + HOLE_WIDTH, break_label, HOLE_WIDTH, string::npos);
				i += break_label.size() - 1;
			}
			continue;
		}
		else if (block[i] == CONTINUE_FLAG) {
			if (continue_label.size() <= HOLE_WIDTH) {
				for (int j = 0; j < continue_label.size(); ++j)
					block[i + j] = continue_label[j];
				i += HOLE_WIDTH - 1;
			}
			else {
				for (int j = 0; j < HOLE_WIDTH; ++j)
					block[i + j] = continue_label[j];
				block.insert(i + HOLE_WIDTH, continue_label, HOLE_WIDTH, string::npos);
				i += continue_label.size() - 1;
			}
		}
	}
}

string ASTNodeIfThenElseStmt::gen_code(GenCodeInfo* gen_code_info) {
	bool block_isover = true;
	bool terminated_bybr;

	stringstream prologue;
	int prev_block, expr_block_begin, expr_block_end;
	int condition_block_begin, condition_block_end, next_block;
	prev_block = gen_code_info->current_block;
	prologue << "  br label %" << gen_code_info->tempval_count << endl;
	expr_block_begin = gen_code_info->tempval_count++;
	gen_code_info->current_block = expr_block_begin;
	prologue << endl;
	prologue << "; <label>:";
	prologue.setf(ios::left, ios::adjustfield);
	prologue.width(40);
	prologue << expr_block_begin << "; preds = %" << prev_block << endl;
	prologue.unsetf(ios::adjustfield);

	vector<pair<string, bool>> body;
	vector<int> expr_block_end_list, next_block_list;
	vector<pair<int, bool>> condition_block_end_list;
	bool break_out = false;
	int tempval_count, current_block;
	vector<int> break_point, continue_point;
	stringstream expr, condition;
	for (int i = 0; i < children.size() - 1; i += 2) {
		int index;
		bool isconstant;
		bool value;
		expr << load_bool(gen_code_info, dynamic_cast<ASTNodeExpression*>(children[i]),
				index, isconstant, value);
		expr_block_end = gen_code_info->current_block;
		if (isconstant) {
			if (value) {
				condition_block_begin = expr_block_begin;
			}
			else {
				tempval_count = gen_code_info->tempval_count;
				current_block = gen_code_info->current_block;
				break_point = gen_code_info->break_point;
				continue_point = gen_code_info->continue_point;
			}
		}
		else {
			expr << "  br i1 %" << index;
			expr << ", label %" << gen_code_info->tempval_count;
			condition_block_begin = gen_code_info->tempval_count++;
			gen_code_info->current_block = condition_block_begin;
		}

		condition << dynamic_cast<ASTNodeBlock*>(children[i + 1])->gen_code(gen_code_info);
		if (!gen_code_info->block_isover)
			condition << "  br label %";
		condition_block_end = gen_code_info->current_block;
		next_block = gen_code_info->tempval_count++;
		gen_code_info->current_block = next_block;

		expr << ", label %" << next_block << endl;
		expr << endl;
		expr << "; <label>:";
		expr.setf(ios::left, ios::adjustfield);
		expr.width(40);
		expr << condition_block_begin << "; preds = %" << expr_block_end << endl;
		expr.unsetf(ios::adjustfield);

		if (break_out)
			continue;
		if (isconstant) {
			if (value) {
				body.push_back(make_pair(condition.str(), gen_code_info->block_isover));
				condition_block_end_list.push_back(
						make_pair(condition_block_end, gen_code_info->block_isover));
				if (!gen_code_info->block_isover)
					block_isover = false;
				else
					terminated_bybr = gen_code_info->terminated_bybr;
				break_out = true;
				tempval_count = gen_code_info->tempval_count;
				current_block = gen_code_info->current_block;
				break_point = gen_code_info->break_point;
				continue_point = gen_code_info->continue_point;
			}
			else {
				gen_code_info->tempval_count = tempval_count;
				gen_code_info->current_block = current_block;
				gen_code_info->break_point = break_point;
				gen_code_info->continue_point = continue_point;
			}
		}
		else {
			body.push_back(make_pair(expr.str(), false));
			body.push_back(make_pair(condition.str(), gen_code_info->block_isover));
			expr_block_end_list.push_back(expr_block_end);
			condition_block_end_list.push_back(
					make_pair(condition_block_end, gen_code_info->block_isover));
			next_block_list.push_back(next_block);
			if (!gen_code_info->block_isover)
				block_isover = false;
			else
				terminated_bybr = gen_code_info->terminated_bybr;
		}
		expr.str("");
		condition.str("");
	}

	stringstream tmp;
	int end_block;
	if (children.size() == 2) {
		// if (expr) {}
		if (body.empty()) {
			gen_code_info->tempval_count--;
			gen_code_info->current_block = prev_block;
			gen_code_info->block_isover = false;
			return "";
		}
		else {
			if (!(body.size() == 1 && block_isover)) {
				if (!block_isover)
					tmp << next_block << endl;
				tmp << endl;
				tmp << "; <label>:";
				tmp.setf(ios::left, ios::adjustfield);
				tmp.width(40);
				tmp << next_block << "; preds = %" << condition_block_end;
				tmp.unsetf(ios::adjustfield);
				if (body.size() != 1)
					tmp << ", %" << expr_block_end;
				tmp << endl;
			}
			if (body.size() == 1) {
				if (block_isover) {
					gen_code_info->tempval_count--;
					gen_code_info->current_block = condition_block_end;
				}
				gen_code_info->block_isover = block_isover;
				gen_code_info->terminated_bybr = terminated_bybr;
				return prologue.str() + body[0].first + tmp.str();
			}
			else {
				gen_code_info->block_isover = false;
				return prologue.str() + body[0].first + body[1].first + tmp.str();
			}
		}
	}
	else {
		string result = prologue.str();
		stringstream elsess;
		bool else_isover;
		if (break_out) {
			// check the correctness of else block
			dynamic_cast<ASTNodeBlock*>(children.back())->gen_code(gen_code_info);
			elsess << body.back().first;
			end_block = current_block;
			else_isover = body.back().second;
			if (block_isover) {
				gen_code_info->current_block = condition_block_end_list.back().first;
				gen_code_info->tempval_count = current_block;
			}
			else {
				gen_code_info->current_block = current_block;
				gen_code_info->tempval_count = tempval_count;
			}
			gen_code_info->break_point = break_point;
			gen_code_info->continue_point = continue_point;
		}
		else {
			elsess << dynamic_cast<ASTNodeBlock*>(children.back())->gen_code(gen_code_info);
			else_isover = gen_code_info->block_isover;
			if (!else_isover) {
				elsess << "  br label %";
				block_isover = false;
			}
			else
				terminated_bybr = gen_code_info->terminated_bybr;
			end_block = gen_code_info->tempval_count;
			condition_block_end_list.push_back(make_pair(gen_code_info->current_block, else_isover));
			if (!block_isover)
				gen_code_info->current_block = gen_code_info->tempval_count++;
		}


		if (block_isover) {
			gen_code_info->block_isover = block_isover;
			gen_code_info->terminated_bybr = terminated_bybr;
		}
		else {
			gen_code_info->block_isover = false;
			if (!else_isover)
				elsess << end_block << endl;
			elsess << endl;
			elsess << "; <label>:";
			elsess.setf(ios::left, ios::adjustfield);
			elsess.width(40);
			elsess << end_block << "; preds = ";
			elsess.unsetf(ios::adjustfield);
			bool first = true;
			for (int i = condition_block_end_list.size() - 1; i >= 0; --i) {
				if (!condition_block_end_list[i].second) {
					if (first) {
						elsess << "%" << condition_block_end_list[i].first;
						first = false;
					}
					else
						elsess << ", %" << condition_block_end_list[i].first;
				}
			}
			elsess << endl;
		}

		for (int i = 0; i < int(body.size()) - 1; i += 2) {
			if (!body[i + 1].second)
				tmp << end_block << endl;
			tmp << endl;
			tmp << "; <label>:";
			tmp.setf(ios::left, ios::adjustfield);
			tmp.width(40);
			tmp << next_block_list[i >> 1] << "; preds = %"
				<< expr_block_end_list[i >> 1] << endl;
			tmp.unsetf(ios::adjustfield);
			result += body[i].first + body[i + 1].first + tmp.str();
			tmp.str("");
		}
		return result + elsess.str();
	}
}

string ASTNodeWhileStmt::gen_code(GenCodeInfo* gen_code_info) {
	stringstream prologue;
	int prev_block, expr_block_begin, expr_block_end;
	int loop_block_begin, loop_block_end, end_block;
	prev_block = gen_code_info->current_block;
	prologue << "  br label %" << gen_code_info->tempval_count << endl;
	expr_block_begin = gen_code_info->tempval_count++;
	gen_code_info->current_block = expr_block_begin;

	int index;
	bool isconstant;
	bool value;
	stringstream expr;
	expr << load_bool(gen_code_info, dynamic_cast<ASTNodeExpression*>(children[0]),
			index, isconstant, value);
	expr_block_end = gen_code_info->current_block;
	if (isconstant) {
		if (value)
			expr << "  br i1 true";
		else
			expr << "  br i1 false";
	}
	else
		expr << "  br i1 %" << index;
	expr << ", label %" << gen_code_info->tempval_count;
	loop_block_begin = gen_code_info->tempval_count++;
	gen_code_info->current_block = loop_block_begin;

	stringstream loop;
	string block;
	bool in_loop = gen_code_info->in_loop;
	vector<int> outer_break_point = gen_code_info->break_point;
	vector<int> outer_continue_point = gen_code_info->continue_point;
	gen_code_info->in_loop = true;
	gen_code_info->break_point.clear();
	gen_code_info->continue_point.clear();
	block = dynamic_cast<ASTNodeBlock*>(children[1])->gen_code(gen_code_info);
	if (!gen_code_info->block_isover)
		loop << "  br label %" << expr_block_begin << endl;
	loop_block_end = gen_code_info->current_block;
	end_block = gen_code_info->tempval_count;

	stringstream ss_break, ss_continue;
	ss_break << end_block;
	ss_continue << expr_block_begin;
	replace_label(block, ss_break.str(), ss_continue.str());

	prologue << endl;
	prologue << "; <label>:";
	prologue.setf(ios::left, ios::adjustfield);
	prologue.width(40);
	prologue << expr_block_begin << "; preds = ";
	prologue.unsetf(ios::adjustfield);
	if (!gen_code_info->block_isover)
		prologue << "%" << loop_block_end << ", ";
	for (int i = gen_code_info->continue_point.size() - 1; i >= 0; --i)
		prologue << "%" << gen_code_info->continue_point[i] << ", ";
	prologue << "%" << prev_block << endl;

	if (isconstant && value)
		expr << ", label %" << loop_block_begin << endl;
	else
		expr << ", label %" << end_block << endl;
	expr << endl;
	expr << "; <label>:";
	expr.setf(ios::left, ios::adjustfield);
	expr.width(40);
	expr << loop_block_begin << "; preds = %" << expr_block_end << endl;
	expr.unsetf(ios::adjustfield);

	if (isconstant && value && gen_code_info->break_point.empty()) {
		if (!gen_code_info->block_isover)
			gen_code_info->terminated_bybr = true;
		// else terminated_bybr doesn't change
		gen_code_info->block_isover = true;
		goto ret;
	}

end:
	gen_code_info->tempval_count++;
	gen_code_info->current_block = end_block;
	gen_code_info->block_isover = false;
	loop << endl;
	loop << "; <label>:";
	loop.setf(ios::left, ios::adjustfield);
	loop.width(40);
	loop << end_block << "; preds = ";
	loop.unsetf(ios::adjustfield);
	for (int i = gen_code_info->break_point.size() - 1; i >= 0; --i)
		loop << "%" << gen_code_info->break_point[i] << ", ";
	loop << "%" << expr_block_end << endl;

ret:
	gen_code_info->in_loop = in_loop;
	gen_code_info->break_point = outer_break_point;
	gen_code_info->continue_point = outer_continue_point;
	return prologue.str() + expr.str() + block + loop.str();
}

string ASTNodeRepeatStmt::gen_code(GenCodeInfo* gen_code_info) {
	stringstream prologue;
	int prev_block, expr_block_begin, expr_block_end;
	int loop_block_begin, loop_block_end, end_block;
	prev_block = gen_code_info->current_block;
	prologue << "  br label %" << gen_code_info->tempval_count << endl;
	loop_block_begin = gen_code_info->tempval_count++;
	gen_code_info->current_block = loop_block_begin;

	stringstream loop;
	string block;
	bool in_loop = gen_code_info->in_loop;
	vector<int> outer_break_point = gen_code_info->break_point;
	vector<int> outer_continue_point = gen_code_info->continue_point;
	gen_code_info->in_loop = true;
	gen_code_info->break_point.clear();
	gen_code_info->continue_point.clear();
	block = dynamic_cast<ASTNodeBlock*>(children[0])->gen_code(gen_code_info);
	if (!gen_code_info->block_isover)
		loop << "  br label %" << gen_code_info->tempval_count << endl;
	loop_block_end = gen_code_info->current_block;
	expr_block_begin = gen_code_info->tempval_count++;
	gen_code_info->current_block = expr_block_begin;

	int index;
	bool isconstant;
	bool value;
	stringstream expr;
	expr << load_bool(gen_code_info, dynamic_cast<ASTNodeExpression*>(children[1]),
			index, isconstant, value);
	expr_block_end = gen_code_info->current_block;
	if (isconstant) {
		if (value)
			expr << "  br i1 true";
		else
			expr << "  br i1 false";
	}
	else
		expr << "  br i1 %" << index;
	expr << ", label %" << gen_code_info->tempval_count;
	end_block = gen_code_info->tempval_count;

	prologue << endl;
	prologue << "; <label>:";
	prologue.setf(ios::left, ios::adjustfield);
	prologue.width(40);
	prologue << loop_block_begin << "; preds = ";
	prologue.unsetf(ios::adjustfield);
	prologue << "%" << expr_block_end << ", %" << prev_block << endl;

	loop << endl;
	loop << "; <label>:";
	loop.setf(ios::left, ios::adjustfield);
	loop.width(40);
	loop << expr_block_begin << "; preds = ";
	loop.unsetf(ios::adjustfield);
	if (!gen_code_info->block_isover) {
		loop << "%" << loop_block_end;
		if (!gen_code_info->continue_point.empty())
			loop << ", ";
	}
	for (int i = gen_code_info->continue_point.size() - 1; i > 0; --i)
		loop << "%" << gen_code_info->continue_point[i] << ", ";
	if (!gen_code_info->continue_point.empty())
		loop << "%" << gen_code_info->continue_point[0];
	loop << endl;

	expr << ", label %" << loop_block_begin << endl;

	stringstream ss_break, ss_continue;
	ss_break << end_block;
	ss_continue << expr_block_begin;
	replace_label(block, ss_break.str(), ss_continue.str());

	if (!gen_code_info->break_point.empty())
		goto end;
	if (gen_code_info->block_isover) {
		if ((gen_code_info->continue_point.empty()) ||
				(isconstant && !value)) {
			gen_code_info->terminated_bybr = true;
			goto ret;
		}
	}
	else {
		if (isconstant && !value) {
			gen_code_info->block_isover = true;
			gen_code_info->terminated_bybr = true;
			goto ret;
		}
	}

end:
	gen_code_info->tempval_count++;
	gen_code_info->current_block = end_block;
	gen_code_info->block_isover = false;
	expr << endl;
	expr << "; <label>:";
	expr.setf(ios::left, ios::adjustfield);
	expr.width(40);
	expr << end_block << "; preds = ";
	expr.unsetf(ios::adjustfield);
	expr << "%" << loop_block_end;
	for (int i = gen_code_info->break_point.size() - 1; i >= 0; --i)
		expr << ", %" << gen_code_info->break_point[i];
	expr << endl;

ret:
	gen_code_info->in_loop = in_loop;
	gen_code_info->break_point = outer_break_point;
	gen_code_info->continue_point = outer_continue_point;
	return prologue.str() + block + loop.str() + expr.str();
}

static string get_parray(GenCodeInfo* gen_code_info, ASTNodeExpression* expr,
		int &index, string &id, ASTNodeType **type) {
	stringstream ss, result;
	string code;
	GenCodeInfo::Result &expr_result = gen_code_info->result;
	code = expr->gen_code(gen_code_info);
	if (gen_code_info->result_type == GenCodeInfo::NONE) {
		ss << gen_code_info->loc << " error: expected value of array type"
			<< ", but get value of type 'void'" << endl;
		throw runtime_error(ss.str());
	}
	else if (gen_code_info->result_type == GenCodeInfo::FUNCTION) {
		ss << gen_code_info->loc << " error: expected value of array type"
			<< ", but get a function" << endl;
		throw runtime_error(ss.str());
	}
	else if (gen_code_info->result_type == GenCodeInfo::POINTER ||
			gen_code_info->result_type == GenCodeInfo::VALUE) {
load_variable:
		if (expr_result.regval.type->variableType() == ASTNodeType::INTEGER) {
			ss << gen_code_info->loc << " error: expected value of array type"
				<< ", but get an integer" << endl;
			throw runtime_error(ss.str());
		}
		else if (expr_result.regval.type->variableType() == ASTNodeType::BOOLEAN) {
		ss << gen_code_info->loc << " error: expected value of array type"
			<< ", but get a boolean value" << endl;
		throw runtime_error(ss.str());
		}
		else if (array_table.find(expr_result.regval.type->getValue()) == array_table.end()){
			ss << gen_code_info->loc << " error: expected value of array type, but get class type '"
				<< expr_result.regval.type->getValue() << "'" << endl;
			throw runtime_error(ss.str());
		}
		// results of GenCodeInfo::VALUE should never reach here
		index = expr_result.regval.index;
		id = expr_result.regval.id;
		*type = expr_result.regval.type;
		code += result.str();
	}
	else {
		ASTNodeExpression* expr = gen_code_info->result.expr;
		if (expr->type() == ASTNode::THIS) {
			ss << gen_code_info->loc << " error: expected value of array type"
				<< ", but get 'this'" << endl;
			throw runtime_error(ss.str());
		}
		else if (expr->type() == ASTNode::STRING) {
			ss << gen_code_info->loc << " error: expected value of array type"
				<< ", but get a string" << endl;
			throw runtime_error(ss.str());
		}
		else if (expr->type() == ASTNode::INTEGER) {
			ss << gen_code_info->loc << " error: expected value of array type"
				<< ", but get an integer" << endl;
			throw runtime_error(ss.str());
		}
		else if (expr->type() == ASTNode::BOOLEAN) {
			ss << gen_code_info->loc << " error: expected value of array type"
				<< ", but get a boolean value" << endl;
			throw runtime_error(ss.str());
		}
		else { //ID
			code += load_id(gen_code_info, expr, expr_result.regval.index,
					&expr_result.regval.id, &expr_result.regval.type);
			goto load_variable;
		}
	}
	return code;
}

string ASTNodeForEachStmt::gen_code(GenCodeInfo* gen_code_info) {
	stringstream ss;
	string iter_id = dynamic_cast<ASTNodeID*>(children[0])->getID();
	ASTNodeType *iter_type;
	string iter_type_asm;
	if (gen_code_info->localvar_table &&
			gen_code_info->localvar_table->find(iter_id) != gen_code_info->localvar_table->end()) {
		iter_type = (*gen_code_info->localvar_table)[iter_id];
	}
	else {
		ss << children[0]->getLoc() << " error: iterator in for-each statement "
			<< "must be a local variable" << endl;
		throw runtime_error(ss.str());
	}
	iter_type_asm = iter_type->getTypeAsm(false);

	int expr_index;
	string expr_id;
	ASTNodeType *expr_type;
	string expr_type_asm;
	stringstream prologue;
	prologue << get_parray(gen_code_info,
			dynamic_cast<ASTNodeExpression*>(children[1]), expr_index, expr_id, &expr_type);
	if (iter_type->getValue() != dynamic_cast<ASTNodeType*>
			(array_table[expr_type->getValue()]->getChildren()[2])->getValue()) {
		ss << children[1]->getLoc() << " error: type of iterator and container not match"
			<< " in for-each statement " << endl;
		throw runtime_error(ss.str());
	}
	expr_type_asm = dynamic_cast<ASTNodeType*>
		(array_table[expr_type->getValue()]->getChildren()[2])->getAsm();

	int count_id;
	int prev_block, expr_block;
	prologue << "  %" << gen_code_info->tempval_count << " = alloca i32, align 4" << endl;
	count_id = gen_code_info->tempval_count++;
	prologue << "  store i32 0, i32* %" << count_id << ", align 4" << endl;
	prologue << "  %" << gen_code_info->tempval_count++ << " = getelementptr inbounds "
		<< expr_type_asm << "* %";
	if (expr_index >= 0)
		prologue << expr_index;
	else
		prologue << expr_id;
	prologue << ", i32 0, i32 0" << endl;
	prologue << "  %" << gen_code_info->tempval_count << " = load "
		<< iter_type_asm << "* %" << gen_code_info->tempval_count - 1
		<< ", align 4" << endl;
	prologue << "  store " << iter_type_asm << " %" << gen_code_info->tempval_count++
		<< ", " << iter_type_asm << "* %" << iter_id << ", align 4" << endl;
	prologue << "  br label %" << gen_code_info->tempval_count << endl;
	prev_block = gen_code_info->current_block;
	expr_block = gen_code_info->tempval_count++;

	stringstream expr;
	int loop_block_begin, loop_block_end;
	expr << "  %" << gen_code_info->tempval_count++ << " = load i32* %"
		<< count_id << ", align 4" << endl;
	expr << "  %" << gen_code_info->tempval_count << " = icmp slt i32 %"
		<< gen_code_info->tempval_count - 1 << ", "
		<< array_table[expr_type->getValue()]->getLength() << endl;
	gen_code_info->tempval_count++;
	expr << "  br i1 %" << gen_code_info->tempval_count - 1
		<< ", label %" << gen_code_info->tempval_count;
	loop_block_begin = gen_code_info->tempval_count++;
	gen_code_info->current_block = loop_block_begin;

	stringstream loop;
	string block;
	int epilogue_block, end_block;
	bool in_loop = gen_code_info->in_loop;
	vector<int> outer_break_point = gen_code_info->break_point;
	vector<int> outer_continue_point = gen_code_info->continue_point;
	gen_code_info->in_loop = true;
	gen_code_info->break_point.clear();
	gen_code_info->continue_point.clear();
	block = dynamic_cast<ASTNodeBlock*>(children[2])->gen_code(gen_code_info);
	if (!gen_code_info->block_isover)
		loop << "  br label %" << gen_code_info->tempval_count << endl;
	loop_block_end = gen_code_info->current_block;
	epilogue_block = gen_code_info->tempval_count++;
	gen_code_info->current_block = epilogue_block;

	stringstream epilogue;
	epilogue << "  %" << gen_code_info->tempval_count++ << " = load i32* %"
		<< count_id << ", align 4" << endl;
	epilogue << "  %" << gen_code_info->tempval_count << " = add nsw i32 %"
		<< gen_code_info->tempval_count - 1 << ", 1" << endl;
	epilogue << "  store i32 %" << gen_code_info->tempval_count++
		<< ", i32* %" << count_id << ", align 4" << endl;
	epilogue << "  %" << gen_code_info->tempval_count++ << " = getelementptr inbounds "
		<< expr_type_asm << "* %";
	if (expr_index >= 0)
		epilogue << expr_index;
	else
		epilogue << expr_id;
	epilogue << ", i32 0, i32 %" << gen_code_info->tempval_count - 2 << endl;
	epilogue << "  %" << gen_code_info->tempval_count << " = load "
		<< iter_type_asm << "* %" << gen_code_info->tempval_count - 1
		<< ", align 4" << endl;
	epilogue << "  store " << iter_type_asm << " %" << gen_code_info->tempval_count++
		<< ", " << iter_type_asm << "* %" << iter_id << ", align 4" << endl;
	epilogue << "  br label %" << expr_block << endl;
	end_block = gen_code_info->tempval_count;

	stringstream ss_break, ss_continue;
	ss_break << end_block;
	ss_continue << epilogue_block;
	replace_label(block, ss_break.str(), ss_continue.str());

	prologue << endl;
	prologue << "; <label>:";
	prologue.setf(ios::left, ios::adjustfield);
	prologue.width(40);
	prologue << expr_block << "; preds = ";
	prologue.unsetf(ios::adjustfield);
	prologue << "%" << epilogue_block << ", %" << prev_block << endl;

	// XXX maybe label %expr_block when cannot proceed?
	expr << ", label %" << end_block << endl;
	expr << endl;
	expr << "; <label>:";
	expr.setf(ios::left, ios::adjustfield);
	expr.width(40);
	expr << loop_block_begin << "; preds = %" << expr_block << endl;
	expr.unsetf(ios::adjustfield);

	loop << endl;
	loop << "; <label>:";
	loop.setf(ios::left, ios::adjustfield);
	loop.width(40);
	loop << epilogue_block << "; preds = ";
	loop.unsetf(ios::adjustfield);
	loop << "%" << loop_block_end;
	for (int i = gen_code_info->continue_point.size() - 1; i >= 0; --i)
		loop << ", %" << gen_code_info->continue_point[i];
	loop << endl;

	if (gen_code_info->break_point.empty() && gen_code_info->continue_point.empty() &&
			gen_code_info->block_isover) {
		gen_code_info->terminated_bybr = true;
		goto ret;
	}

end:
	gen_code_info->tempval_count++;
	gen_code_info->current_block = end_block;
	gen_code_info->block_isover = false;
	epilogue << endl;
	epilogue << "; <label>:";
	epilogue.setf(ios::left, ios::adjustfield);
	epilogue.width(40);
	epilogue << end_block << "; preds = ";
	epilogue.unsetf(ios::adjustfield);
	for (int i = gen_code_info->break_point.size() - 1; i >= 0; --i)
		epilogue << "%" << gen_code_info->break_point[i] << ", ";
	epilogue << "%" << expr_block << endl;

ret:
	gen_code_info->in_loop = in_loop;
	gen_code_info->break_point = outer_break_point;
	gen_code_info->continue_point = outer_continue_point;
	return prologue.str() + expr.str() + block + loop.str() + epilogue.str();
}
