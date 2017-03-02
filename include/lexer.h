
#ifndef LEXER_H_
#define LEXER_H_

#include <vector>

#include "types.h"
#include "nfa.h"
#include "dfa.h"

// The regular expression documentation used by the lexer can be found in regex.h
// 
// When adding multiple tokenize rules to the lexer, there will likely be conflicts
// which have to be solved by two rules in the particular order:
//  
//  1) The longest possible matching token will be chosen.
//     Example: [0-9]+ expression on content "51262" will match the whole sequence
//              instead of "5", "1", ..., "2".
//
//  2) The token for a rule added earlier to the lexing ruleset will be chosen.
//
// Note: 2) Only applies when there is a match of the same length for multiple
//          tokenize rules at the 

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
