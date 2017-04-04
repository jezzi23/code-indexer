
#include "lexer.h"

#include <cstring>
#include <vector>
#include <iostream>
#include <cassert>

#include "file_mapped_io.h"
#include "regex.h"
#include "finite_automata.h"
#include "utils.h"

internal_ inline void resolveLineTracking(LexingIterator& lex_data); 
internal_ inline void setTokenStartAsNext(LexingIterator& lex_data); 
internal_ inline void setTokenStartAsCurrent(LexingIterator& lex_data); 

void
resolveLineTracking(LexingIterator& lex_data) {
  if (*lex_data.itr == '\n') {
    ++lex_data.line_count;
    lex_data.last_line_begin = lex_data.itr;
  }
}

void
setTokenStartAsNext(LexingIterator& lex_data) {
  lex_data.token_begin = lex_data.itr + 1;
  lex_data.token_line = lex_data.line_count;

  if (*lex_data.token_begin != '\n') {
    lex_data.token_column = lex_data.token_begin - lex_data.last_line_begin;
  } else {
    ++lex_data.token_line;
    lex_data.token_column = 0;
  }
}

void
setTokenStartAsCurrent(LexingIterator& lex_data) {
  lex_data.token_begin = lex_data.itr;
  lex_data.token_line = lex_data.line_count;
  lex_data.token_column = lex_data.token_begin - lex_data.last_line_begin;
}


Token::Token() {

}

Token::Token(LexingIterator lex_itr, int token_id) :
    index(lex_itr.token_begin - lex_itr.begin),
    length(lex_itr.itr - lex_itr.token_begin),
    id(token_id),
    line_count(lex_itr.token_line),
    column_count(lex_itr.token_column) {

}

Lexer::Lexer() :
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

void
Lexer::setStream(const char* input_data_begin, const char* input_data_end) {
  lexing_data = {input_data_begin, input_data_begin, input_data_end,
                 input_data_begin - 1, 1,
                 input_data_begin, 1, 1};
}

const char*
Lexer::begin() {
  return lexing_data.begin;
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
  std::vector<unsigned int> tmp_set;
  std::vector<unsigned int> current_state_set = {nfa->begin_state};
  current_state_set = nfa->epsilonSearch(current_state_set);
  // Save state of longest match so far of token and lexing_state
  Token longest_match_so_far;
  longest_match_so_far.id = 0;


  // Can rewind to lexing state after last token match or to simply skip
  // meaningless characters
  // TODO: Might have to deal with line increments here
  LexingIterator rewind_lexing_state = lexing_data;
    if (rewind_lexing_state.token_line == 52) {
      int breakme = 5;
    }
  setTokenStartAsNext(rewind_lexing_state);

  for (;
       ;
       ++lexing_data.itr) {

    if (lexing_data.itr == lexing_data.end) {
      if (rewind_lexing_state.itr < lexing_data.itr) {

        rewindBackTo(rewind_lexing_state, current_state_set);
        continue;

      } else {
        break;
      }
    }
    
    if (lexing_data.line_count == 124) {
      int breakme = 5;
    }
    resolveLineTracking(lexing_data);

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

    if (is_garbage_set) {
      if (longest_match_so_far.id != 0) {
        // rewind back to where the token was found
        lexing_data = rewind_lexing_state;
        return longest_match_so_far;

      } else {

        rewindBackTo(rewind_lexing_state, current_state_set);
        continue;
      }
    } else if (governing_token != 0){
      longest_match_so_far = Token(lexing_data, governing_token);
      // set rewind state to continue after where token ends
      rewind_lexing_state = lexing_data;
      setTokenStartAsCurrent(rewind_lexing_state);
    }

    tmp_set.clear();
    for (auto current_state : current_state_set) {
      unsigned int transition_state = nfa->transition(current_state,
                                                      *lexing_data.itr);
      if (transition_state != nfa->garbage_state) {
        tmp_set.push_back(transition_state);
      }
    }
    // branch out after
    current_state_set = nfa->epsilonSearch(tmp_set);
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

void
Lexer::rewindBackTo(LexingIterator& rewind_data,
                    std::vector<unsigned int>& state_set) {
  
  lexing_data = rewind_data;
  ++rewind_data.itr;
  resolveLineTracking(rewind_data);
  setTokenStartAsNext(rewind_data);

  state_set.clear();
  state_set.push_back(nfa->begin_state);
  state_set = nfa->epsilonSearch(state_set);

}
