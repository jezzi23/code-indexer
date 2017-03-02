
#ifndef LEXER_H_
#define LEXER_H_

#include <vector>

#include "types.h"
#include "nfa.h"
#include "dfa.h"

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

class Lexer {
public:
  // Regex syntax is POSIX EXTENDED.
  // https://en.wikipedia.org/wiki/Regular_expression#POSIX_basic_and_extended
  Lexer(const char* input_data_begin, const char* input_data_end);
  ~Lexer();
  
  void addRule(const Regexpr regexpr, int token_id);
  void build();

  Token nextToken();
  void rewind();  
private:
  LexingIterator lexing_data; 
  // DFA states
  // Order of rules added impact priority
  unsigned int simulateChar(const char letter);

  enum class LexingState : u8 {
    INITIALIZATION_PHASE,
    BUILD_PHASE,
    QUERY_PHASE
  } status;

  // Lexer internally constructs NFA during build phase
  // which gets replaced with a DFA for lexing phase.
  union {
    DFA<unsigned int, int, 1 << 7>* dfa;
    NFA<unsigned int, int, 1 << 7>* nfa;
  };
};

#endif // LEXER_H_
