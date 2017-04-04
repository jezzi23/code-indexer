
#ifndef CPP_LEXER_H_
#define CPP_LEXER_H_

void buildCppLexer();
void feedLexer(const char* file_begin, const char* file_end);

void lexerTestPrintAllTokens();
void lexerTestPrintAllFunctionsCalledInFunctions();

#endif // CPP_LEXER_H_

