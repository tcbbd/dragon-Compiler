%top{
	#include <stdio.h>
	#include <iostream>
	#include <sstream>
	using namespace std;
	extern int yyerror(const char*);
	int linenum = 0;
}


%%

"program"		fprintf(yyout, "<PROGRAM>");
"var"			fprintf(yyout, "<VAR>");
"type"			fprintf(yyout, "<TYPE>");
"is array of"	fprintf(yyout, "<ISARRAYOF>");
"function"		fprintf(yyout, "<FUNCTION>");
"end function"	fprintf(yyout, "<ENDFUNCTION>");
"is class"		fprintf(yyout, "<ISCLASS>");
"is"			fprintf(yyout, "<IS>");
"end class"		fprintf(yyout, "<CLASSDEC_TAL>");
"extends"		fprintf(yyout, "<EXTENDS>");


"do"			fprintf(yyout, "<DO>");
"while"			fprintf(yyout, "<WHILE>");
"end while"		fprintf(yyout, "<ENDWHILE>");
"repeat"		fprintf(yyout, "<REPEAT>");
"until"			fprintf(yyout, "<UNTIL>");
"foreach"		fprintf(yyout, "<FOREACH>");
"in"			fprintf(yyout, "<IN>");
"end foreach"	fprintf(yyout, "<ENDFOREACH>");

"if"			fprintf(yyout, "<IF>");
"then"			fprintf(yyout, "<THEN>");
"else"			fprintf(yyout, "<ELSE>");
"elif"			fprintf(yyout, "<ELIF>");	
"end if"		fprintf(yyout, "<ENDIF>");
		
"return"		fprintf(yyout, "<RETURN>");
"break"			fprintf(yyout, "<BREAK>");
"continue"		fprintf(yyout, "<CONTINUE>");
"print"			fprintf(yyout, "<PRINT>");

[-+*/%]			fprintf(yyout, "<OP, %s>", yytext);
"=="			fprintf(yyout, "<OP, ==>");
":="			fprintf(yyout, "<OP, ASSIGN>");
">"				fprintf(yyout, "<OP, >>");
"<"				fprintf(yyout, "<OP, <>");
">="			fprintf(yyout, "<OP, >=>");
"<="			fprintf(yyout, "<OP, <=");
"or"			fprintf(yyout, "<OR>");
"and"			fprintf(yyout, "<AND>");

"yes"			fprintf(yyout, "<BOOL, \"YES\">");
"no"			fprintf(yyout, "<BOOL, \"NO\">");
0				fprintf(yyout, "<INT, 0>");
[1-9][0-9]*		fprintf(yyout, "<INT, %s>", yytext);

";"				fprintf(yyout, "<;>");
","				fprintf(yyout, "<,>");
"."				fprintf(yyout, "<.>");
"integer"		fprintf(yyout, "<INTEGER>");
"boolean"		fprintf(yyout, "<BOOLEAN>");
"begin"			fprintf(yyout, "<BEGIN>");
"end"			fprintf(yyout, "<END>");
"("				fprintf(yyout, "<(>");
")"				fprintf(yyout, "<)>");
"["				fprintf(yyout, "<[>");
"]"				fprintf(yyout, "<]>");

[a-zA-Z_$][a-zA-Z0-9_$]*	fprintf(yyout, "<ID, \"%s\">", yytext);

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
	fprintf(yyout, "<STRING, %s>", ss.str().data());
}
"//".*$			fprintf(yyout, "<COMMENT, %s>", yytext + 2);	
[ \t]			; /* ignore whitespace */
\n		{
	linenum++;
	fprintf(yyout, "\n");
}
.			{ fprintf(yyout, "\n%d: lexical error\n", linenum); return 0; }
%%

int yywrap()
{
	return 1;
}

int main(int argc, char **argv){
	if (argc >= 2)
		yyin = fopen(argv[1], "r");
	yyout = fopen(argv[2],"w");
	yylex();
	
	fprintf(yyout, "Line Number: %d\n.", linenum);
	fclose(yyin);
	fclose(yyout);
	return 0;
}
