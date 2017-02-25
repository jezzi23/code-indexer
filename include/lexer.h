
#ifndef LEXER_H_
#define LEXER_H_

#include <vector>
#include <map>

#include "types.h"

struct LexingIterator {
  const char* begin;
  const char* itr;
  const char* end;

  const char* last_line_begin;
  unsigned int line_count;

  const char* token_begin;
  unsigned int token_line;
  unsigned int token_column;
};

enum TokenIdentifier : int {
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
  Token();
  Token(LexingIterator lex_itr, int token_id);

  u64 index;
  int length;
  int id;
  int line_count;
  int column_count;
};

//std::vector<Token> tokenize(const char* regex);

class Lexer {
public:
  // Regex syntax is POSIX EXTENDED.
  // https://en.wikipedia.org/wiki/Regular_expression#POSIX_basic_and_extended
  Lexer(const char* input_data_begin, const char* input_data_end);
  ~Lexer();
  
  void addRule(const char* regex, int token_id);
  void build();

  Token nextToken();
  void rewind();  
private:
  LexingIterator lexing_data; 
  // DFA states
  // Order of rules added impact priority
  unsigned int estimateNumStates(const char* regex);
  unsigned int simulateChar(const char letter);

  // Lexer internally constructs NFA during build phase
  // which gets replaced with a DNA for lexing phase.
  union {
    DFA* dfa;
    NFA* nfa;
  };
};

#endif // LEXER_H_
