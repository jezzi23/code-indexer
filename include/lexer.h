
#ifndef LEXER_H_
#define LEXER_H_

#include <vector>

#include "types.h"

struct Token {
  u64 index;
  int length;
  int id;
  int line_count;
  int column_count;
};

enum class TokenType {
  OPEN_PARANTHESIS = '(',
  CLOSE_PARANTHESIS = ')',
  OPEN_BRACKET = '{',
  CLOSE_BRACKET = '}',
  OPEN_SQUARE_BRACKET = '[',
  CLOSE_SQUARE_BRACKET = ']',
  COMMENT,
  NAME
};

std::vector<Token> tokenize(const char* filename);  

#endif // LEXER_H_
