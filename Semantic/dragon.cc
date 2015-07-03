#include <cstdio>
#include <fstream>
#include "ast.h"
#include "dragon.tab.hh"

extern FILE *yyin;
extern ASTNode *ast_root;

int main(int argc, char **argv) {
	if (argc <= 3) {
		cout << "Usage: " << argv[0] << " source AST_output llvm_asm_output" << endl;
		return 1;
	}

	yyin = fopen(argv[1], "r");
	streambuf *saved_cout = cout.rdbuf();
	ofstream output;
	output.open(argv[2], output.out | output.trunc);
	cout.rdbuf(output.rdbuf());

	yy::parser parser;
	//parser.set_debug_level(1);
	parser.parse();
	if (ast_root)
		ast_root->traverse_draw_terminal(0);

	cout.rdbuf(saved_cout);
	output.close();

	saved_cout = cout.rdbuf();
	output.open(argv[3], output.out | output.trunc);
	cout.rdbuf(output.rdbuf());

	try {
		if (ast_root) {
			ast_root->collect_info();
			dynamic_cast<ASTNodeProgram*>(ast_root)->gen_code();
		}
	}
	catch (runtime_error &e) {
		cout.rdbuf(saved_cout);
		output.close();
		output.open(argv[3], output.out | output.trunc);
		output << e.what();
		output.close();
		return -1;
	}

	cout.rdbuf(saved_cout);
	output.close();
	return 0;
}
