
#include "lexer.h"

#include <cstring>
#include <vector>
#include <iostream>
#include <cassert>

#include "file_mapped_io.h"
#include "regex.h"
#include "finite_automata.h"
#include "utils.h"

Token::Token() {

}

Token::Token(LexingIterator lex_itr, int token_id) :
    index(lex_itr.token_begin - lex_itr.begin),
    length(lex_itr.itr - lex_itr.token_begin),
    id(token_id),
    line_count(lex_itr.token_line),
    column_count(lex_itr.token_column) {

}

Lexer::Lexer(const char* lexing_data_begin, const char* lexing_data_end) :
    lexing_data{lexing_data_begin, lexing_data_begin, lexing_data_end,
                lexing_data_begin - 1, 1,
                lexing_data_begin, 1, 1},
    status(LexingState::INITIALIZATION_PHASE),
    nfa(nullptr) {

}

Lexer::~Lexer() {
  switch (status) {
    case LexingState::BUILD_PHASE : {
      nfa->~NFA();
      break;
    }
    case LexingState::QUERY_PHASE : {
//      dfa->~DFA();
      break;
    }
    default: {
      break;
    }
  }
}

void
Lexer::addRule(const Regexpr regexpr, int token_id) {
  if (status == LexingState::INITIALIZATION_PHASE) {
    status = LexingState::BUILD_PHASE;
    nfa = new NFA<unsigned int, int, 1<<7>;
  }
  assert(status == LexingState::BUILD_PHASE);

  std::vector<unsigned int> tokenized_states =
    nfa->addExprGroup(regexpr.expr_begin,
                      regexpr.expr_end,
                      std::vector<unsigned int>{nfa->begin_state},
                      ExpressionGroupQuantification(1, 1));

  bool already_contains_token_higher_prio = false;
  for (auto final_state : tokenized_states) {
	  if (nfa->stateType(final_state) != 0) {
	    already_contains_token_higher_prio = true;
	  }
  }
  if (!already_contains_token_higher_prio) {
	  for (auto final_state : tokenized_states) {
	    nfa->writeStateType(final_state, token_id);
	  }
  }
}

void
Lexer::build() {
  // TODO: actually convert nfa to dfa
  status = LexingState::QUERY_PHASE;
}

/*
DFA querying will look something like this 

Token
Lexer::nextToken() {
  assert(status == LexingState::QUERY_PHASE);
  unsigned int state = dfa.begin_state;

  while (lexing_data.itr != lexing_data.end) {
    if (*lexing_data.itr == '\n') {
      ++lexing_data.line_count;
      lexing_data.last_line_begin = lexing_data.itr;
    }
    if (state == dfa.begin_state) {
      // start over
      lexing_data.token_begin = lexing_data.itr;
      lexing_data.token_line = lexing_data.line_count;
      lexing_data.token_column = lexing_data.itr - lexing_data.last_line_begin;
    }
    state = dfa.states[state * dfa.alphabet_size + (*lexing_data.itr)];
    ++lexing_data.itr;

    auto accept_itr = dfa.accept_states.find(state);
 
    if (accept_itr != dfa.accept_states.end()) {
      return Token(lexing_data, accept_itr->second);
    }
  }
  return Token(lexing_data, TokenIdentifier::END_OF_FILE);
}
*/

Token
Lexer::nextToken() {
  assert(status == LexingState::QUERY_PHASE);
  std::vector<unsigned int> current_state_set = {nfa->begin_state};
  // Save state of longest match so far of token and lexing_state
  Token longest_match_so_far;
  longest_match_so_far.id = 0;
  LexingIterator after_token_match_lexing_state;
  

  while (lexing_data.itr != lexing_data.end) {
    if (*lexing_data.itr == '\n') {
      ++lexing_data.line_count;
      lexing_data.last_line_begin = lexing_data.itr;
    }

    // branch out before to all epsilon transitions stored in current_state_set
    current_state_set = nfa->epsilonSearch(current_state_set);
    std::vector<unsigned int> next_state_set;
    for (auto current_state : current_state_set) {
      unsigned int transition_state = nfa->transition(current_state,
                                                      *lexing_data.itr);
      if (transition_state != nfa->garbage_state) {
        next_state_set.push_back(transition_state);
      }
    }
    // branch out after
    current_state_set = nfa->epsilonSearch(next_state_set);

    ++lexing_data.itr;

    bool is_garbage_set = true;
    int governing_token = 0;

    for (auto current_state : current_state_set) {
      if (current_state != nfa->garbage_state) {
        is_garbage_set = false;
      }
      const int state_type = nfa->stateType(current_state);
      if (state_type != nfa->garbage_state) {
        governing_token = state_type;
      }
    }

    if (!is_garbage_set) {
      if (governing_token != 0) {
        longest_match_so_far = Token(lexing_data, governing_token);
        after_token_match_lexing_state = {
            lexing_data.begin, 
            lexing_data.itr, 
            lexing_data.end, 
            lexing_data.last_line_begin, 
            lexing_data.line_count, 
            lexing_data.itr,
            lexing_data.line_count, 
            static_cast<unsigned int>(
              lexing_data.itr - lexing_data.last_line_begin + 1)};
      }
    } else {
      if (longest_match_so_far.id != 0) {
        // rewind back to where the token was found
        lexing_data = after_token_match_lexing_state;
        return longest_match_so_far;
      } else {
        lexing_data.token_begin = lexing_data.itr;
        lexing_data.token_line = lexing_data.line_count;
        lexing_data.token_column =
          lexing_data.itr - lexing_data.last_line_begin;

        current_state_set.clear();
        current_state_set.push_back(nfa->begin_state);
      }
    }
  }

  return Token(lexing_data, -52); // will be end of file (EOF)
}

void
Lexer::rewind() {
  lexing_data.itr = lexing_data.begin;

  lexing_data.last_line_begin = lexing_data.begin - 1;
  lexing_data.token_begin = lexing_data.begin;

  lexing_data.line_count = 1;
}

