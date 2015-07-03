%code top{
	#include <cstdio>
	#include <fstream>
	#include "dragon.h"
}

%require "3.0"
%language "C++"
%locations
%error-verbose
/*%define parse.trace
%debug*/

%code{
	extern int yylex(yy::parser::semantic_type *yylval, yy::parser::location_type *yylloc);
	extern FILE *yyin;
	ASTNode *ast_root;
}
%define api.value.type variant

%token <ASTNodeInteger *> INTEGER
%token <ASTNodeBoolean *> BOOLEAN
%token <ASTNodeString *> STRING
%type <ASTNodeLiteral *> literal

%token <ASTNodeID *> ID
%token <ASTNodeType *> TYPE_INT TYPE_BOOL
%type <ASTNodeType *> type

%right ASSIGN
%left OR
%left AND
%left EQ NE
%left LE GE '<' '>'
%left SL SR
%left '+' '-'
%left '*' '/' '%'

%type <ASTNodeExpression *> expr
%type <ASTNodeExpressionList *> expr_list
%type <ASTNodeExpressionList *> expr_list_
%type <ASTNodePrimary *> primary
%type <ASTNodeFieldAccess *> field_access
%type <ASTNodeArrayAccess *> array_access
%type <ASTNodeMethodInvocation *> method_invocation

%token IF THEN ELSE ELIF ENDIF
%token DO WHILE ENDWHILE REPEAT UNTIL FOREACH IN ENDFOREACH
%token RETURN BREAK CONTINUE PRINT
%type <ASTNodeStatement *> stmt
%type <ASTNodeElifList *> elif_list
%type <ASTNodeBlock *> block
%type <ASTNodeBlock *> block_

/* Prevent from the conflict with flex macro BEGIN*/
%token VAR TYPE IS ISARRAYOF BEGINN END
%token FUNCTION ENDFUNCTION
%token ISCLASS ENDCLASS EXTENDS
%type <ASTNodeParameterList *> param_list
%type <ASTNodeParameterList *> param_list_
%type <ASTNodeVariableDecl *> variable_decl
%type <ASTNodeVariableDeclList *> variable_decls
%type <ASTNodeVariableDeclList *> variable_decls_
%type <ASTNodeFunctionDecl *> function_decl
%type <ASTNodeFunctionDefn *> function_defn
%type <ASTNodeArrayDecl *> array_decl
%type <ASTNodeClassBody *> class_body
%type <ASTNodeClassBody *> class_body_
%type <ASTNodeClassDecl *> class_decl

%token PROGRAM
%type <ASTNodeDeclaration *> compound_decl
%type <ASTNodeCompoundDeclList *> compound_decls
%type <ASTNodeCompoundDeclList *> compound_decls_
%type <ASTNodeProgram *> program

%%
program:
	PROGRAM ID '(' ')' compound_decls IS variable_decls BEGINN block END {
		$$ = new ASTNodeProgram($2, $5, $7, $9);
		ast_root = $$;
	}
;

compound_decls:
	%empty { $$ = new ASTNodeCompoundDeclList(NULL); }
|	compound_decls_ { $$ = $1; }
;

compound_decls_:
	compound_decl { $$ = new ASTNodeCompoundDeclList($1); }
|	compound_decls_ compound_decl { $$ = new ASTNodeCompoundDeclList($1, $2); }
;

compound_decl:
	function_decl { $$ = $1; }
|	function_defn { $$ = $1; }
|	class_decl { $$ = $1; }
|	array_decl { $$ = $1; }
;

class_decl:
	TYPE ID ISCLASS class_body ENDCLASS ';' { 
		$$ = new ASTNodeClassDecl($2, new ASTNodeType(ASTNodeType::VOID), $4);
	}
|	TYPE ID ISCLASS EXTENDS type class_body ENDCLASS ';' {
		$$ = new ASTNodeClassDecl($2, $5, $6);
	}
;

class_body:
	%empty { $$ = new ASTNodeClassBody((ASTNodeVariableDecl *)NULL);}
|	class_body_ { $$ = $1; }
;

class_body_:
	variable_decl { $$ = new ASTNodeClassBody($1); }
|	function_defn { $$ = new ASTNodeClassBody($1); }
|	class_body_ variable_decl { $$ = new ASTNodeClassBody($1, $2); }
|	class_body_ function_defn { $$ = new ASTNodeClassBody($1, $2); }
;

array_decl:
	TYPE ID ISARRAYOF expr type ';' { $$ = new ASTNodeArrayDecl($2, $4, $5); }
;

function_decl:
	FUNCTION ID '(' param_list ')' variable_decls ENDFUNCTION ID ';' {
		if ($2->getID() != $8->getID())
			YYABORT;
		$$ = new ASTNodeFunctionDecl($2, $4, $6, new ASTNodeType(ASTNodeType::VOID));
	}
|	FUNCTION ID '(' param_list ')' variable_decls RETURN type ';' ENDFUNCTION ID ';' {
		if ($2->getID() != $11->getID())
			YYABORT;
		$$ = new ASTNodeFunctionDecl($2, $4, $6, $8);
	}
;

function_defn:
	FUNCTION ID '(' param_list ')' variable_decls IS variable_decls BEGINN
		block ENDFUNCTION ID ';' {
		if ($2->getID() != $12->getID())
			YYABORT;
		$$ = new ASTNodeFunctionDefn($2, $4, $6, new ASTNodeType(ASTNodeType::VOID), $8, $10);
	}
|	FUNCTION ID '(' param_list ')' variable_decls RETURN type ';' IS
		variable_decls BEGINN block ENDFUNCTION ID ';' {
		if ($2->getID() != $15->getID())
			YYABORT;
		$$ = new ASTNodeFunctionDefn($2, $4, $6, $8, $11, $13);
	}
;

variable_decls:
	%empty { $$ = new ASTNodeVariableDeclList(NULL); }
|	variable_decls_ { $$ = $1; }
;

variable_decls_:
	variable_decl { $$ = new ASTNodeVariableDeclList($1); }
|	variable_decls_ variable_decl { $$ = new ASTNodeVariableDeclList($1, $2); }
;

variable_decl:
	VAR ID IS type ';' { $$ = new ASTNodeVariableDecl($2, $4); }
;

param_list:
	%empty { $$ = new ASTNodeParameterList(NULL); }
|	param_list_ { $$ = $1; }
;

param_list_:
	ID { $$ = new ASTNodeParameterList($1); }
|	param_list_ ',' ID { $$ = new ASTNodeParameterList($1, $3); }
;

block:
	%empty { $$ = new ASTNodeBlock(NULL); }
|	block_ { $$ = $1; }
;

block_:
	stmt { $$ = new ASTNodeBlock($1); }
|	block_ stmt { $$ = new ASTNodeBlock($1, $2); }
;

elif_list:
	ELIF expr THEN block { $$ = new ASTNodeElifList($2, $4); }
|	elif_list ELIF expr THEN block { $$ = new ASTNodeElifList($1, $3, $5); }
;

stmt:
	IF expr THEN block ENDIF { $$ = new ASTNodeIfThenElseStmt($2, $4, NULL); }
|	IF expr THEN block ELSE block ENDIF { $$ = new ASTNodeIfThenElseStmt($2, $4, $6); }
|	IF expr THEN block elif_list ELSE block ENDIF { $$ = new ASTNodeIfThenElseStmt($2, $4, $5, $7); }
|	WHILE expr DO block ENDWHILE { $$ = new ASTNodeWhileStmt($2, $4); }
|	REPEAT block UNTIL expr ';' { $$ = new ASTNodeRepeatStmt($2, $4); }
|	FOREACH ID IN expr DO block ENDFOREACH { $$ = new ASTNodeForEachStmt($2, $4, $6); }
|	BREAK ';' { $$ = new ASTNodeBreakStmt(); }
|	CONTINUE ';' { $$ = new ASTNodeContinueStmt(); }
|	RETURN ';' { $$ = new ASTNodeReturnStmt(NULL); }
|	RETURN expr ';' { $$ = new ASTNodeReturnStmt($2); }
|	PRINT expr ';' { $$ = new ASTNodePrintStmt($2); }
|	expr ';' { $$ = new ASTNodeExpressionStmt($1); }
|	';' { $$ = new ASTNodeExpressionStmt(NULL); }
;

expr:
	ID ASSIGN expr { $$ = new ASTNodeBinaryExpr($1, $3, ":="); }
|	field_access ASSIGN expr { $$ = new ASTNodeBinaryExpr($1, $3, ":="); }
|	array_access ASSIGN expr { $$ = new ASTNodeBinaryExpr($1, $3, ":="); }
|	expr OR expr { $$ = new ASTNodeBinaryExpr($1, $3, "or");}
|	expr AND expr { $$ = new ASTNodeBinaryExpr($1, $3, "and");}
|	expr EQ expr { $$ = new ASTNodeBinaryExpr($1, $3, "==");}
|	expr NE expr { $$ = new ASTNodeBinaryExpr($1, $3, "!=");}
|	expr LE expr { $$ = new ASTNodeBinaryExpr($1, $3, "<=");}
|	expr GE expr { $$ = new ASTNodeBinaryExpr($1, $3, ">=");}
|	expr '<' expr { $$ = new ASTNodeBinaryExpr($1, $3, "<");}
|	expr '>' expr { $$ = new ASTNodeBinaryExpr($1, $3, ">");}
|	expr SL expr { $$ = new ASTNodeBinaryExpr($1, $3, "<<");}
|	expr SR expr { $$ = new ASTNodeBinaryExpr($1, $3, ">>");}
|	expr '+' expr { $$ = new ASTNodeBinaryExpr($1, $3, "+");}
|	expr '-' expr { $$ = new ASTNodeBinaryExpr($1, $3, "-");}
|	expr '*' expr { $$ = new ASTNodeBinaryExpr($1, $3, "*");}
|	expr '/' expr { $$ = new ASTNodeBinaryExpr($1, $3, "/");}
|	expr '%' expr { $$ = new ASTNodeBinaryExpr($1, $3, "%");}
/* We must assign explicitly, otherwise bison will report a type clash.*/
|	primary { $$ = $1; }
;

expr_list:
	%empty { $$ = new ASTNodeExpressionList(NULL); }
|	expr_list_ { $$ = $1; }
;

expr_list_:
	expr { $$ = new ASTNodeExpressionList($1); }
|	expr_list_ ',' expr { $$ = new ASTNodeExpressionList($1, $3); }
;

primary:
	'(' expr ')' { $<ASTNodeExpression *>$ = $2; }
|	literal { $$ = $1; }
|	ID { $$ = $1;}
|	field_access { $$ = $1; }
|	array_access { $$ = $1; }
|	method_invocation { $$ = $1; }
;

field_access:
	primary '.' ID { $$ = new ASTNodeFieldAccess($1, $3); }
;

array_access:
	primary '[' expr ']' { $$ = new ASTNodeArrayAccess($1, $3); }
;

method_invocation:
	ID '(' expr_list ')' { $$ = new ASTNodeMethodInvocation($1, $3); }
|	field_access '(' expr_list ')' { $$ = new ASTNodeMethodInvocation($1, $3); }
;

literal:
	INTEGER { $$ = $1; }
|	BOOLEAN { $$ = $1; }
|	STRING { $$ = $1; }
;

type:
	TYPE_INT { $$ = new ASTNodeType("integer"); }
|	TYPE_BOOL { $$ = new ASTNodeType("boolean"); }
|	ID	{ $$ = new ASTNodeType($1->getID()); }
;
%%

void yy::parser::error(const yy::parser::location_type& L, const string& M) {
	cout << L << ' ' << M << endl;
}

int main(int argc, char **argv) {
	if (argc > 1)
		yyin = fopen(argv[1], "r");
	streambuf *saved_cout = cout.rdbuf();
	ofstream output;
	if (argc > 2) {
		output.open(argv[2]);
		cout.rdbuf(output.rdbuf());
	}
	yy::parser parser;
	//parser.set_debug_level(1);
	parser.parse();
	if (ast_root)
		ast_root->traverse_draw_terminal(0);
	cout.rdbuf(saved_cout);
	if (argc > 2)
		output.close();
	return 0;
}
