%top{
	#include <stdio.h>
	#include "ast.h"
	#include "dragon.tab.hh"
	
	#define YY_DECL int yylex(yy::parser::semantic_type *yylval, yy::parser::location_type *yylloc)
	#define YY_USER_ACTION yylloc->columns(yyleng);
}

%%
%{
	yylloc->step();
%}
"program"		return yy::parser::token::PROGRAM;
"var"			return yy::parser::token::VAR;
"type"			return yy::parser::token::TYPE;
"is array of"	return yy::parser::token::ISARRAYOF;
"function"		return yy::parser::token::FUNCTION;
"end function"	return yy::parser::token::ENDFUNCTION;
"is class"		return yy::parser::token::ISCLASS;
"end class"		return yy::parser::token::ENDCLASS;
"extends"		return yy::parser::token::EXTENDS;
"is"			return yy::parser::token::IS;

"if"			return yy::parser::token::IF;
"then"			return yy::parser::token::THEN;
"else"			return yy::parser::token::ELSE;
"elif"			return yy::parser::token::ELIF;	
"end if"		return yy::parser::token::ENDIF;

"do"			return yy::parser::token::DO;
"while"			return yy::parser::token::WHILE;
"end while"		return yy::parser::token::ENDWHILE;
"repeat"		return yy::parser::token::REPEAT;
"until"			return yy::parser::token::UNTIL;
"foreach"		return yy::parser::token::FOREACH;
"in"			return yy::parser::token::IN;
"end foreach"	return yy::parser::token::ENDFOREACH;

"return"		return yy::parser::token::RETURN;
"break"			return yy::parser::token::BREAK;
"continue"		return yy::parser::token::CONTINUE;
"print"			return yy::parser::token::PRINT;

	/* Prevent from the conflict with flex macro BEGIN */
"begin"			return yy::parser::token::BEGINN;
"end"			return yy::parser::token::END;

":="			return yy::parser::token::ASSIGN;
"or"			return yy::parser::token::OR;
"and"			return yy::parser::token::AND;
"=="			return yy::parser::token::EQ;
"!="			return yy::parser::token::NE;
">="			return yy::parser::token::GE;
"<="			return yy::parser::token::LE;
"<<"			return yy::parser::token::SL;
">>"			return yy::parser::token::SR;

"integer"		return yy::parser::token::TYPE_INT;
"boolean"		return yy::parser::token::TYPE_BOOL;

	/* literal */
"this"	{
	yylval->build(new ASTNodeThis());
	yylval->as<ASTNodeThis*>()->setLoc(*yylloc);
	return yy::parser::token::THIS;
}
"yes"	{
	yylval->build(new ASTNodeBoolean(true));
	yylval->as<ASTNodeBoolean*>()->setLoc(*yylloc);
	return yy::parser::token::BOOLEAN;
}
"no"	{
	yylval->build(new ASTNodeBoolean(false));
	yylval->as<ASTNodeBoolean*>()->setLoc(*yylloc);
	return yy::parser::token::BOOLEAN;
}
0|[1-9][0-9]*	{
	yylval->build(new ASTNodeInteger(atoi(yytext)));
	yylval->as<ASTNodeInteger*>()->setLoc(*yylloc);
	return yy::parser::token::INTEGER;
}
[a-zA-Z_$][a-zA-Z0-9_$]*	{
	yylval->build(new ASTNodeID(yytext));
	yylval->as<ASTNodeID*>()->setLoc(*yylloc);
	return yy::parser::token::ID;
}

\"([^"\\]|\\.)*\"	{
	stringstream ss;
	for (int i = 1; i < yyleng - 1; i++) {
		if (*(yytext + i) == '\\') {
			i++;
			switch (*(yytext + i)) {
				case '\\': case '"':
					ss << *(yytext + i);
					break;
				case 'a':
					ss << '\a';
					break;
				case 'b':
					ss << '\b';
					break;
				case 'f':
					ss << '\f';
					break;
				case 'n':
					ss << '\n';
					break;
				case 'r':
					ss << '\r';
					break;
				case 't':
					ss << '\t';
					break;
				case 'v':
					ss << '\v';
					break;
				default:
					ss << '\\' << *(yytext + i);
					break;
			}
		}
		else
			ss << *(yytext + i);
	}
	yylval->build(new ASTNodeString(ss.str()));
	yylval->as<ASTNodeString*>()->setLoc(*yylloc);
	return yy::parser::token::STRING;
}
"//".*$			/* ignore comment */
[ \t]+			{ yylloc->step(); }
("\n"|"\r\n")	{ yylloc->lines(1); yylloc->step(); }
.				return *yytext;
%%

int yywrap()
{
	return 1;
}
