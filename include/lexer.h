
#ifndef LEXER_H_
#define LEXER_H_

#include <vector>

#include "types.h"

struct LexingIterator {
  const char* begin;
  const char* end;
  const char* itr;

  const char* last_line_begin;
  int line_count;
}

class Token {
public:
  Token(LexingIterator lex_itr, TokenIdentifier id, int length);

private:
  u64 index;
  int length;
  TokenIdentifier id;
  int line_count;
  int column_count;
};

enum class TokenIdentifier {
  OPEN_PARANTHESIS = '(',
  CLOSE_PARANTHESIS = ')',
  OPEN_BRACKET = '{',
  CLOSE_BRACKET = '}',
  OPEN_SQUARE_BRACKET = '[',
  CLOSE_SQUARE_BRACKET = ']',
  COMMENT,
  NAME,
  LITERAL_STRING,
  LITERAL_CHAR,
  LITERAL_INT,
  LITERAL_FLOAT,
  EOF
};

std::vector<Token> tokenize(const char* filename);  

#endif // LEXER_H_
