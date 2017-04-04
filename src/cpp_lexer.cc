
// Print test for the lexer.

#include <cstdio>

#include "lexer.h"
#include "index_database.h"
#include "file_mapped_io.h"

internal_ Lexer cpp_lexer;
internal_ Token token;

enum {
  OCCUPIED = 0,
  PREPROCESSOR_DIRECTIVES,
  NATIVE_BOOL_TYPES,
  NATIVE_CHAR_TYPES,
  NATIVE_INTEGER_TYPES,
  NATIVE_FLOAT_TYPES,
  WHITE_SPACE_FOOD,
  INTEGER_LITERAL,
  FLOAT_LITERAL,
  NAME,
  COMMENT,
  DELIMITER,
  KEYWORD,
  END_OF_FILE = -52
};

void
buildCppLexer() {

  // Example rules
  // Note: C++11 raw string is useful for regex descriptions.

  // C-style comment
  cpp_lexer.addRule(R"(/\*(\*[^/]|[^*])*\*/)", COMMENT);

  cpp_lexer.addRule(R"(for|while|if|else if|switch)", KEYWORD);

  cpp_lexer.addRule(R"(#define)", PREPROCESSOR_DIRECTIVES);
  
  cpp_lexer.addRule(R"(bool)",
                NATIVE_CHAR_TYPES);

  cpp_lexer.addRule(R"(char|wchar_t|char16_t|char32_t|)"
                R"(unsigned char|unsigned wchar_t|)"
                R"(unsigned char16_t|unsigned char32_t)",

                NATIVE_CHAR_TYPES);
  
  cpp_lexer.addRule(R"((signed )?short|(signed )?short int|)"
                R"(unsigned short|unsigned short int|)"

                R"((signed )?int|)"
                R"(unsigned int|)"

                R"((signed )?long|(signed )?long int|)"
                R"(unsigned long|unsigned long int|)"

                R"((signed )?long long|(signed )?long long int|)"
                R"(unsigned long long|unsigned long long int|)"

                R"((std::)?u?int(8|16|32|64)_t|)"
                R"((std::)?u?int_(least|fast)?(8|16|32|64)_t|)"
                R"((std::)?u?intmax_t|)"
                R"((std::)?u?intptr_t)",

                NATIVE_INTEGER_TYPES);

  cpp_lexer.addRule(R"(float|(long )?double)", NATIVE_FLOAT_TYPES);

  // Floating point literals may be suffixed with f or l
  cpp_lexer.addRule(R"([0-9]*\.[0-9]+[FfLl])", FLOAT_LITERAL);

  // Integer literals may be suffixed with u and l or ll may follow
  cpp_lexer.addRule(R"([0-9]+[Uu]?[Ll]{,2})", INTEGER_LITERAL);

  cpp_lexer.addRule(R"([a-zA-Z_]+)", NAME);

  cpp_lexer.addRule(R"({|}|\(|\)|,|;|:{1,2}|\[|\]|<|>|\.)", DELIMITER);

  //cpp_lexer.addRule("\n|\r|\t| ", WHITE_SPACE_FOOD);

  cpp_lexer.build();
}

void
feedLexer(const char* file_begin, const char* file_end) {
  cpp_lexer.setStream(file_begin, file_end);
}

void
lexerTestPrintAllTokens() {

  Token token;
  int count = 0;

  for (;;) {

    token = cpp_lexer.nextToken();
    if (token.id == END_OF_FILE) {
      std::cout << "EOF reached" << std::endl;
      break;
    }

    count++;

    std::cout << "Token found."                       << '\n';
    std::cout << "Index:\t"     << token.index        << '\n';
    std::cout << "Length:\t"    << token.length       << '\n';
    std::cout << "Id:\t"        << token.id           << '\n';
    std::cout << "Line:\t"      << token.line_count   << '\n';
    std::cout << "Column:\t"    << token.column_count << std::endl;
    printf("Contents:\n%.*s\n\n",
           token.length,
           cpp_lexer.begin() + token.index);
  }
  std::cout << "Total occurrences: " << count << std::endl;

  cpp_lexer.rewind();
}

// Top-down recursive parsing helper calls to find functions
internal_ void
inFunctionScope() {

  int bracket_depth = 1;

  while (bracket_depth != 0) {
    token = cpp_lexer.nextToken();
    switch (*(cpp_lexer.begin() + token.index)) {
      case '{': {
        ++bracket_depth;
        break;
      }
      case '}': {
        --bracket_depth;
        break;
      }
      default:
        if (token.id == NAME) {

          int line = token.line_count;
          const char* func_begin = cpp_lexer.begin() + token.index;

          token = cpp_lexer.nextToken();
          if (*(cpp_lexer.begin() + token.index) == '(') {

            for (;;) {
              token = cpp_lexer.nextToken();

              if (*(cpp_lexer.begin() + token.index) == ')') {

                const char* func_end = cpp_lexer.begin() + token.index;
                int func_len = 1 + func_end - func_begin;

                std::cout << "Found function call on line " << line << std::endl;
                printf("Contents:\n%.*s\n\n",
                       func_len,
                       func_begin);
                break;
              }
              if (*(cpp_lexer.begin() + token.index) != ',' &&
                  token.id != NAME) {
                break;
              }
            }
          }
        }
        break;
    }
  }

  return;
}

void
lexerTestPrintAllFunctionsCalledInFunctions() {

  while (token.id != END_OF_FILE) {
    token = cpp_lexer.nextToken();

    if (token.id == DELIMITER &&
        *(cpp_lexer.begin() + token.index) == '{') {
      inFunctionScope();
    }
  }
  std::cout << "EOF reached." << std::endl;
  cpp_lexer.rewind();
}

