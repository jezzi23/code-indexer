
#ifndef LEXER_H_
#define LEXER_H_

#include <vector>

#include "types.h"
#include "nfa.h"

// The regular expression documentation used by the lexer can be found
// in regex.h
// 
// When adding multiple tokenize rules to the lexer, there will likely be
// conflicts which have to be solved by two rules in the particular order:
//  
//  1) The longest possible matching token will be chosen.
//     Example: [0-9]+ expression on content "51262" will match the whole
//              sequence instead of "5", "1", ..., "2".
//
//  2) In case of a tie, the token for a rule added earlier
//     to the lexing ruleset will be chosen.

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
  unsigned int length;
  int id;
  unsigned int line_count;
  unsigned int column_count;
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

  enum class LexingState : u8 {
    INITIALIZATION_PHASE,
    BUILD_PHASE,
    QUERY_PHASE
  } status;

  // Lexer internally constructs NFA during build phase
  // which gets replaced with a DFA for lexing phase.
  union {
//  DFA<unsigned int, int, 1 << 7>* dfa;
    NFA<unsigned int, int, 1 << 7>* nfa;
  };
};

#endif // LEXER_H_

