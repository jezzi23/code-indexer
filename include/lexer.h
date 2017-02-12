
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

  const char* current_token_begin;
  int current_token_column;
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
  END_OF_FILE
};

class Token {
public:
  Token(LexingIterator lex_itr, TokenIdentifier id, int token_length);

  u64 index;
  int length;
  TokenIdentifier id;
  int line_count;
  int column_count;
};

std::vector<Token> tokenize(const char* filename);  

#endif // LEXER_H_
