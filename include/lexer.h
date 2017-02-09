
#ifndef LEXER_H_
#define LEXER_H_

#include <vector>

#include "types.h"

struct Token {
  u64 index;
  int length;
  int id;
}

std::vector<Token> tokenize(const char* filename);  

#endif // LEXER_H_
