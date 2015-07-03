%code top{
	#include "ast.h"
}

%require "3.0"
%language "C++"
%locations
%error-verbose
/*%define parse.trace
%debug*/

%code{
	extern int yylex(yy::parser::semantic_type *yylval, yy::parser::location_type *yylloc);
	ASTNode *ast_root;
}
%define api.value.type variant

%token <ASTNodeInteger *> INTEGER
%token <ASTNodeBoolean *> BOOLEAN
%token <ASTNodeString *> STRING
%type <ASTNodeLiteral *> literal

%token <ASTNodeID *> ID
%token <ASTNodeThis *> THIS
%token <ASTNodeType *> TYPE_INT TYPE_BOOL
%type <ASTNodeType *> type

%right ASSIGN
%left OR
%left AND
%left '|'
%left '^'
%left '&'
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
		$$->setLoc(@$);
		ast_root = $$;
	}
;

compound_decls:
	%empty { $$ = new ASTNodeCompoundDeclList(); $$->setLoc(@$); }
|	compound_decls_ { $$ = $1; $$->setLoc(@$); }
;

compound_decls_:
	compound_decl { $$ = new ASTNodeCompoundDeclList(); $$->append($1); $$->setLoc(@$); }
|	compound_decls_ compound_decl { $$ = $1; $$->append($2); $$->setLoc(@$); }
;

compound_decl:
	function_defn { $$ = $1; $$->setLoc(@$); }
|	class_decl { $$ = $1; $$->setLoc(@$); }
|	array_decl { $$ = $1; $$->setLoc(@$); }
;

class_decl:
	TYPE ID ISCLASS class_body ENDCLASS ';' { 
		$$ = new ASTNodeClassDecl($2, new ASTNodeType(ASTNodeType::VOID), $4);
		$$->setLoc(@$);
	}
|	TYPE ID ISCLASS EXTENDS type class_body ENDCLASS ';' {
		$$ = new ASTNodeClassDecl($2, $5, $6);
		$$->setLoc(@$);
	}
;

class_body:
	%empty { $$ = new ASTNodeClassBody(); $$->setLoc(@$); }
|	class_body_ { $$ = $1; $$->setLoc(@$); }
;

class_body_:
	variable_decl { $$ = new ASTNodeClassBody(); $$->append($1); $$->setLoc(@$); }
|	function_defn { $$ = new ASTNodeClassBody(); $$->append($1); $$->setLoc(@$); }
|	class_body_ variable_decl { $$ = $1; $$->append($2); $$->setLoc(@$); }
|	class_body_ function_defn { $$ = $1; $$->append($2); $$->setLoc(@$); }
;

array_decl:
	TYPE ID ISARRAYOF expr type ';' { $$ = new ASTNodeArrayDecl($2, $4, $5); $$->setLoc(@$); }
;

function_defn:
	FUNCTION ID '(' param_list ')' variable_decls IS variable_decls BEGINN
		block ENDFUNCTION ID ';' {
		if ($2->getID() != $12->getID())
			YYABORT;
		$$ = new ASTNodeFunctionDefn($2, $4, $6, new ASTNodeType(ASTNodeType::VOID), $8, $10);
		$$->setLoc(@$);
	}
|	FUNCTION ID '(' param_list ')' variable_decls RETURN type ';' IS
		variable_decls BEGINN block ENDFUNCTION ID ';' {
		if ($2->getID() != $15->getID())
			YYABORT;
		$$ = new ASTNodeFunctionDefn($2, $4, $6, $8, $11, $13);
		$$->setLoc(@$);
	}
;

variable_decls:
	%empty { $$ = new ASTNodeVariableDeclList(); $$->setLoc(@$); }
|	variable_decls_ { $$ = $1; $$->setLoc(@$); }
;

variable_decls_:
	variable_decl { $$ = new ASTNodeVariableDeclList(); $$->append($1); $$->setLoc(@$); }
|	variable_decls_ variable_decl { $$ = $1; $$->append($2); $$->setLoc(@$); }
;

variable_decl:
	VAR ID IS type ';' { $$ = new ASTNodeVariableDecl($2, $4); $$->setLoc(@$); }
;

param_list:
	%empty { $$ = new ASTNodeParameterList(); $$->setLoc(@$); }
|	param_list_ { $$ = $1; $$->setLoc(@$); }
;

param_list_:
	ID { $$ = new ASTNodeParameterList(); $$->append($1); $$->setLoc(@$); }
|	param_list_ ',' ID { $$ = $1; $$->append($3); $$->setLoc(@$); }
;

block:
	%empty { $$ = new ASTNodeBlock(); $$->setLoc(@$); }
|	block_ { $$ = $1; $$->setLoc(@$); }
;

block_:
	stmt { $$ = new ASTNodeBlock(); $$->append($1); $$->setLoc(@$); }
|	block_ stmt { $$ = $1; $$->append($2); $$->setLoc(@$); }
;

elif_list:
	ELIF expr THEN block { $$ = new ASTNodeElifList($2, $4); $$->setLoc(@$); }
|	elif_list ELIF expr THEN block { $$ = $1; $$->append($3, $5); $$->setLoc(@$); }
;

stmt:
	IF expr THEN block ENDIF { $$ = new ASTNodeIfThenElseStmt($2, $4, NULL); $$->setLoc(@$); }
|	IF expr THEN block ELSE block ENDIF { $$ = new ASTNodeIfThenElseStmt($2, $4, $6); $$->setLoc(@$); }
|	IF expr THEN block elif_list ELSE block ENDIF { $$ = new ASTNodeIfThenElseStmt($2, $4, $5, $7); $$->setLoc(@$); }
|	WHILE expr DO block ENDWHILE { $$ = new ASTNodeWhileStmt($2, $4); $$->setLoc(@$); }
|	REPEAT block UNTIL expr ';' { $$ = new ASTNodeRepeatStmt($2, $4); $$->setLoc(@$); }
|	FOREACH ID IN expr DO block ENDFOREACH { $$ = new ASTNodeForEachStmt($2, $4, $6); $$->setLoc(@$); }
|	BREAK ';' { $$ = new ASTNodeBreakStmt(); $$->setLoc(@$); }
|	CONTINUE ';' { $$ = new ASTNodeContinueStmt(); $$->setLoc(@$); }
|	RETURN ';' { $$ = new ASTNodeReturnStmt(); $$->setLoc(@$); }
|	RETURN expr ';' { $$ = new ASTNodeReturnStmt($2); $$->setLoc(@$); }
|	PRINT expr_list ';' { $$ = new ASTNodePrintStmt($2); $$->setLoc(@$); }
|	expr ';' { $$ = new ASTNodeExpressionStmt($1); $$->setLoc(@$); }
|	';' { $$ = new ASTNodeExpressionStmt(); $$->setLoc(@$); }
;

expr:
	expr ASSIGN expr { $$ = new ASTNodeBinaryExpr($1, $3, ":="); $$->setLoc(@$); }
|	expr OR expr { $$ = new ASTNodeBinaryExpr($1, $3, "or"); $$->setLoc(@$); }
|	expr AND expr { $$ = new ASTNodeBinaryExpr($1, $3, "and"); $$->setLoc(@$); }
|	expr '|' expr { $$ = new ASTNodeBinaryExpr($1, $3, "|"); $$->setLoc(@$); }
|	expr '^' expr { $$ = new ASTNodeBinaryExpr($1, $3, "^"); $$->setLoc(@$); }
|	expr '&' expr { $$ = new ASTNodeBinaryExpr($1, $3, "&"); $$->setLoc(@$); }
|	expr EQ expr { $$ = new ASTNodeBinaryExpr($1, $3, "=="); $$->setLoc(@$); }
|	expr NE expr { $$ = new ASTNodeBinaryExpr($1, $3, "!="); $$->setLoc(@$); }
|	expr LE expr { $$ = new ASTNodeBinaryExpr($1, $3, "<="); $$->setLoc(@$); }
|	expr GE expr { $$ = new ASTNodeBinaryExpr($1, $3, ">="); $$->setLoc(@$); }
|	expr '<' expr { $$ = new ASTNodeBinaryExpr($1, $3, "<"); $$->setLoc(@$); }
|	expr '>' expr { $$ = new ASTNodeBinaryExpr($1, $3, ">"); $$->setLoc(@$); }
|	expr SL expr { $$ = new ASTNodeBinaryExpr($1, $3, "<<"); $$->setLoc(@$); }
|	expr SR expr { $$ = new ASTNodeBinaryExpr($1, $3, ">>"); $$->setLoc(@$); }
|	expr '+' expr { $$ = new ASTNodeBinaryExpr($1, $3, "+"); $$->setLoc(@$); }
|	expr '-' expr { $$ = new ASTNodeBinaryExpr($1, $3, "-"); $$->setLoc(@$); }
|	expr '*' expr { $$ = new ASTNodeBinaryExpr($1, $3, "*"); $$->setLoc(@$); }
|	expr '/' expr { $$ = new ASTNodeBinaryExpr($1, $3, "/"); $$->setLoc(@$); }
|	expr '%' expr { $$ = new ASTNodeBinaryExpr($1, $3, "%"); $$->setLoc(@$); }
|	primary { $$ = $1; $$->setLoc(@$); }
;

expr_list:
	%empty { $$ = new ASTNodeExpressionList(); $$->setLoc(@$); }
|	expr_list_ { $$ = $1; $$->setLoc(@$); }
;

expr_list_:
	expr { $$ = new ASTNodeExpressionList(); $$->append($1); $$->setLoc(@$); }
|	expr_list_ ',' expr { $$ = $1; $$->append($3); $$->setLoc(@$); }
;

primary:
	'(' expr ')' { $<ASTNodeExpression *>$ = $2; $$->setLoc(@$); }
|	literal { $$ = $1; $$->setLoc(@$); }
|	ID { $$ = $1; $$->setLoc(@$); }
|	THIS { $$ = $1; $$->setLoc(@$); }
|	field_access { $$ = $1; $$->setLoc(@$); }
|	array_access { $$ = $1; $$->setLoc(@$); }
|	method_invocation { $$ = $1; $$->setLoc(@$); }
;

field_access:
	primary '.' ID { $$ = new ASTNodeFieldAccess($1, $3); $$->setLoc(@$); }
;

array_access:
	primary '[' expr ']' { $$ = new ASTNodeArrayAccess($1, $3); $$->setLoc(@$); }
;

method_invocation:
	ID '(' expr_list ')' { $$ = new ASTNodeMethodInvocation($1, $3); $$->setLoc(@$); }
|	field_access '(' expr_list ')' { $$ = new ASTNodeMethodInvocation($1, $3); $$->setLoc(@$); }
;

literal:
	INTEGER { $$ = $1; $$->setLoc(@$); }
|	BOOLEAN { $$ = $1; $$->setLoc(@$); }
|	STRING { $$ = $1; $$->setLoc(@$); }
;

type:
	TYPE_INT { $$ = new ASTNodeType("integer"); $$->setLoc(@$); }
|	TYPE_BOOL { $$ = new ASTNodeType("boolean"); $$->setLoc(@$); }
|	ID	{ $$ = new ASTNodeType($1->getID()); $$->setLoc(@$); }
;
%%

void yy::parser::error(const yy::parser::location_type& L, const string& M) {
	cout << L << ' ' << M << endl;
}
