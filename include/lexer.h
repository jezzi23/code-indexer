
#ifndef LEXER_H_
#define LEXER_H_

#include <vector>

#include "types.h"

struct LexingIterator {
  const char* begin;
  const char* itr;
  const char* end;

  const char* last_line_begin;
  const char* current_token_begin;

  unsigned int line_count;
  unsigned int current_token_column;
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

std::vector<Token> tokenize(const char* regex);

class Lexer {
public:
  // Regex syntax is POSIX EXTENDED.
  // https://en.wikipedia.org/wiki/Regular_expression#POSIX_basic_and_extended
  Lexer(const char* regex, const char* input_data, unsigned int input_length);


  Token nextToken();
  void reset();  
private:
  LexingIterator lexing_data; 
  // DFA states
  void buildDFA(const char* regex);
  unsigned int estimateNumStates(const char* regex);
  unsigned int simulateChar(const char letter);
  bool isFinishState(unsigned int state);

  struct {
    //dfa_states[0] is start state
    const unsigned int alphabet_size = 1 << 7;
    unsigned int* states;
    unsigned int* final_states;
    unsigned int num_states;
    
    unsigned int current_state;
  } dfa;
};

#endif // LEXER_H_
