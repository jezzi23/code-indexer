
#include "lexer.h"

#include <regex>
#include <cstring>
#include <cerrno>
#include <vector>
#include <iostream>
#include <cassert>
#include <stdio.h>

#include "file_mapped_io.h"
#include "finite_automata.h"

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
    nfa(nullptr) {
}

Lexer::~Lexer() {
  operator delete(dfa.states);
}

Token
Lexer::nextToken() {
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

    // possibly continue until we find the longest match or state_begin?
    if (state != 0) {
      int breakme = 5;
    }
    auto accept_itr = dfa.accept_states.find(state);
 
    if (accept_itr != dfa.accept_states.end()) {
      return Token(lexing_data, accept_itr->second);
    }
  }
  return Token(lexing_data, TokenIdentifier::END_OF_FILE);
}

void
Lexer::rewind() {
  lexing_data.itr = lexing_data.begin;

  lexing_data.last_line_begin = lexing_data.begin - 1;
  lexing_data.token_begin = lexing_data.begin;

  lexing_data.line_count = 1;
}

// TODO: How to deal with |, e.g. (abc)* | bca | cb+
/*
unsigned int
Lexer::estimateNumStates(const char* regex) {
  unsigned int total_states = 1; // always have start state 
  
  for (const char* itr = regex; *itr != '\0'; ++itr) {
    switch (*itr) {
      // deal with special metacharacters 
      case '*': {
        // don't need a new state for * metacharacter
        --total_states;
        continue;
      }
      case '+': {
        
        continue;
      }
      case '[': {
        // only need one state until we find a matchin ']' 
        // move the iterator to that position and continue
        do {
          ++itr;
          if (*itr == '\0') {
            std::cerr << "Bad regular expression: no matching ']' for '['" << std::endl;
          }
        } while (*itr != ']');
        break;
      }
      case '\\': {
        // we escape the next metacharacter so just walk over it by one
        ++itr;
        break;
      }
      case '(': {
        // a group... continue to next character
        continue;
      }
      case ')': {
        // a closing group... continue to next character
        continue;
      }
      default: {
        // normal character
        break;
      }
    }
    ++total_states; 
  }
  return total_states; 
}
*/

// Expression group quantification:
//
// "*" : zero or more occurrences
// "+" : one or more occurrences 
// "?" : zero or one occurences 
// "{n}" : exactly n occurrences
// "{n,}" : n or more occurrences
// "{n,m}": between n and m occurences (inclusive)
// "{,m}" : between zero and m occurences
internal_ 
struct ExpressionGroupQuantification {
  ExpressionGroupQuantification(int min, int max) : min_occurrences(min), max_occurrences(max) {

  }
  ExpressionGroupQuantification(const char* quantifier_begin) {
    bool is_quantified = quantifyOnString(quantification_begin);
    if (!is_quantified) {
      // No quantification, e.g. (abcd) as opposed to (abcd)*
      // -> appear exactly once
      min_occurrences = max_occurrences = 1;
    }
  }
  
  bool quantifyOnString(const char* quantification_begin) {
    switch (*quantifier_begin) {
      case '*': {
        min_occurrences = 0;
        max_occurrences = INFINITE_OCCURRENCES;
        return true;
      }
      case '+': {
        min_occurrences = 1;
        max_occurrences = INFINITE_OCCURRENCES;
        return true;
      }
      case '?': {
        min_occurrences = 0;
        max_occurrences = 1;
        return true;
      }
      case '{': {
        const size_t num_str_max_length = 10;
        char atoi_buffer[num_str_max_length + 1] = { 0 };

        const char* quantifier_itr = quantifier_begin + 1;
        const char* num_begin = quantifier_itr;
        u8 num_count = 0;

        // walk through {....} and write min -or max_occurrences
        for (;;) {
          if (*quantifier_itr == '}' || *quantifier_itr == ',') {
            ++num_count; // we have attempted to read a number from string range
            const size_t parsed_num_length = quantifier_itr - num_begin;
            if (parsed_num_length > num_str_max_length) {
              return false; // number value in string is too long
            }
            memcpy(atoi_buffer, num_begin, parsed_num_length);
            errno = 0;
            int num_val = strtol(atoi_buffer, nullptr, 10);
            if (errno == ERANGE && parsed_num_length != 0) {
              // strtol failed because atoi_buffer is not a valid number as a string
              // "" string of length 0 is allowed for default values
              return false;
            }
            memset(atoi_buffer, 0, parsed_num_length); // reset buffer

            switch (*quantifier_itr) {
              case '}': {
                assert(num_count >= 1 && num_count <= 2);
                if (num_count <= 1) {
                  // we are looking at {n} exact match number
                  if (parsed_num_length == 0) {
                    // empty {} not allowed
                    return false;
                  } else {
                    max_occurrences = min_occurrences = num_val;
                  }
                } else {
                  // we are looking at {n,m} where n or m can be empty ""
                  // and m is the value num_val
                  max_occurrences = num_val;
                }
                return true;
              }
              case ',': {
                assert(num_count == 1);
                if (parsed_num_length == 0) {
                  min_occurrences = 0;
                } else {
                  min_occurrences = num_val;
                }
                break;
              }
            }
          } else if (*quantifier_itr == '\0') {
            return false;
          }
          ++quantifier_itr;
        }
      }
    }
    // if we make it to here, no known quantifier was found
    return false;
  }

  int min_occurrences;
  int max_occurrences;

  const int INFINITE_OCCURRENCES = -1;
};

std::vector<unsigned int>&
Lexer::addExprGroup(const char* regex_group_begin,
                    const char* regex_group_end,
                    std::vector<unsigned int> state_start_set,
                    const ExpressionGroupQuantification grp_quantification);
  //  [regex_begin ; regex_end[ is a complete sub expression to be baked into state_start.
  //  At this point in time, the expression can contain children sub expression(s).
  //    e.g.
  //    "(abc) | (dfg)" contains sub expressions abc and dfg
  //    "abc(dfg)*((hjk)+)|(qw)" contains sub expressions (dfg)*, ((hjk)+|(wq))
  //
  //  Recursively find all sub-expressions with a depth-first approach.

  std::vector<unsigned int> current_state_set = state_start_set;
  unsigned int              current_governing_state = garbage_state;

  std::vector<unsigned int> next_state_set;
  unsigned int              next_governing_state;

  // The state set where the cycle begins, for e.g. *, +, or {n,m}
  std::vector<unsigned int> cycle_connectivity_state_set;
  std::vector<unsigned int> group_final_state_set;
  //unsigned int target_state; // the state we wish to be in after reading [regex_begin;regex_end[

  bool last_group_in_expr;

  bool transition_buffer[1<<7];
  //TODO: Figure out correct loop patterns. for {0, ~} - {5, 7} etc
  for (int occurrence_count = min_occurences;
       occurrence_count < min_occurences || occurrence_count < max_group_occurrences
                                         || max_group_occurrences == INFINITE_OCCURRENCES;
       ++occurrence_count) {

    if (occurrence_count == min_occurences && max_group_occurrences == INFINITE_OCCURRENCES) {
      // For '+', '*' and '{n,} expression type
      // The current state set will be used for cycle connectivity
      cycle_connectivity_state_set = current_state_set;
    }

    for (const char* regex_itr = regex_begin;
         regex_itr != regex_end;
         ++regex_itr) {

      memset(transition_buffer, 0, sizeof(*transition_buffer));
      current_state_set = std::move(next_state_set);
      next_state_set = std::vector<unsigned int>();

      if (regex_itr + 1 == regex_end) {
        last_group_in_expr = true;
      }

      switch (*regex_itr) {
        
        case '\\': {
          // we escape the next metacharacter so just walk over it by one
          // and interpret it as a char value
          ++itr;
          transition_buffer[*itr] = true;
          break;
        }
        case '(': {
          // Treat the contents of (...) as a subexpression and resolve the outcome
          // depending on whether there was a (...)*, (...)+ or just (...)
          
          // find matching ')' on same depth (nested) level.  
          +regex_itr;
          const char* sub_expr_begin = *regex_itr;
          const char* sub_expr_end;

          for (int depth = 1;
               depth != 0;
               ++regex_itr) {

            if (*regex_itr == '\0') {
              std::cerr << "Bad regular expression: no matching ')' for '('" << std::endl;
            }
            depth += *regex_itr == ')' ? -1 : 1;
          }
          sub_expr_end = regex_itr - 1;
          // the current state becomes the continuation state retuned from addSubExpr(...)
          // TODO: Finish group matching 
          next_state_set = addExprGroup(sub_expr_begin,
                                        sub_expr_end,
                                        current_state_set,
                                        ExpressionGroupQuantification(regex_itr));
          continue;
        }
        case ')': {
          std::cerr << "Bad regular expression: no matching '(' for ')'" << std::endl;
          break;
        }
        case '|': {
          //TODO: Save current final states, and continue from here with start state
          break;
        }
        case '[': {
          // only need one state until we find a matchin ']' 
          // move the iterator to that position and continue
          const char* look_ahead = regex_itr + 1;
          const char* first_char = look_ahead;
          bool flip_transition = false;
          if (*look_ahead == '^') {
            flip_transition = true;
            ++look_ahead;
            for (unsigned char i = 0; i < 128; ++i) {
              transition_buffer.push_back(i);
            }
          }
          // buffer the first value in case we see range based expression
          // e.g. [A-Z], store 'A' in first_val buffer
          char first_val;
          while (*look_ahead != ']') {
            switch (*look_ahead) {
              case '\0': {
                std::cerr << "Bad regular expression: no matching ']' for '['" << std::endl;
              }
              case '-': {
                ++look_ahead;
                if (*look_ahead == '\\') {
                  ++look_ahead;
                }
                char end_val = *look_ahead; 
                if (end_val < first_val) { 
                  // swap to allow [a-z] to have same semantic meaning as [z-a]
                  char tmp = end_val;
                  end_val = first_val;
                  first_val = tmp;
                }
                for (char val = first_val; val <= end_val; ++val) {
                  if (flip_transition) {
                    transition_buffer.erase(std::lower_bound(transition_buffer.begin(),
                                                             transition_buffer.end(),
                                                             val));
                  } else {
                    transition_buffer.push_back(val);
                  }
                }
                break;
              }
              default: {
                first_val = *look_ahead;
                if (flip_transition) {
                  transition_buffer.erase(std::lower_bound(transition_buffer.begin(),
                                                           transition_buffer.end(),
                                                           first_val));
                } else {
                  transition_buffer.push_back(first_val);
                }
              }
            }
            ++look_ahead;
          }
          itr = look_ahead;
          break;
        }
        case '.': {
          // can match any character
          for (unsigned char i = 0; i < 128; ++i) {
            transition_buffer.push_back(i);
          }
          break;
        }
        default: {
          // Normal character. Look ahead character quantification (e.g. a*, a+ ,a?, a{n,m}
          
          transition_buffer[*itr] = true;
          break;
        }
      }
      
      if (last_group_in_expr &&
          min_occurences == occurrence_count - 1 &&
          cycle_connectivity_state_set.size() != 0) {
        // time to do cycle connectivity

            // next transition goes to begin_state
        for (auto cycle_state : cycle_connectivity_state_set) {
          for (auto current_state : current_state_set) { 
            for (auto char_transition : transition_buffer) { 

              while (dfa.transition(current_state, char_transition) != cycle_state) {
                 
                if (dfa.transition(current_state, char_transition) == dfa.garbage_state) { 
                  // unused state transition so we freely write to it
                  dfa.writeTransition(cycle_state, current_state, char_transition);
                } else {
                  // there was already a state transition, so we peek into it and attempt
                  // so write the
                  

                  }
                }                                                                                
              }
            }                                                                                  
          }                                                                                    
        }
      } else {
        // not last group. ordinary writes
        
        for (auto current_state : current_state_set) { 
          for (auto char_transition : transition_buffer) { 

            unsigned int state_peek = dfa.transition(current_state, char_transition);
            if (state_peek == dfa.garbage_state) { 
              unsigned int new_state = dfa.makeState();
              // unused state transition so we freely write to it
              dfa.writeTransition(new_state, current_state, char_transition);
              next_state_set.push_back(new_state);
            } else if (state_peek == current_state) {
              // extend the self-loop by one state and copy its contents
              unsigned int new_state = dfa.makeState();
              dfa.copyState(new_state, state_peek);
              dfa.writeTransitions(new_state, current_state, char_transition);
              next_state_set.push_back(new_state);
              
            } else {
              // there was already a state transition to another one, so we peek into it
              next_state_set.push_back(state_peek); 
            }
          }                                                                                  
        }                                                                                    
      }
    }
  }
}

void Lexer::addRule(const char* regex, int token_id) {
  
  //dfa.num_states = estimateNumStates(regex);
  std::vector<unsigned int> accept_states = addSubExpr(regex,
                                                       regex + strlen(regex),
                                                       {dfa.begin_state});
  for (unsigned int accept_state : accept_states) {
    if (dfa.stateType(accept_state) == 0) {
      dfa.writeStateType(accept_state, token_id);
    }
    // else: a token_id has already been written to accept_state
    //       that has a higher priority so we just yield and do nothing
  }
}
