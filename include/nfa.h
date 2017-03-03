
#ifndef NFA_H_
#define NFA_H_

#include <vector>
#include <cstring>
#include <iostream>
#include <algorithm>

#include "finite_automata.h"
#include "regex.h"

template <typename S, typename T, size_t a_size>
class NFA : private FiniteAutomata<S, T, a_size> {
public: 
  NFA();
  ~NFA();

  std::vector<S> addExprGroup(const char* regex_group_begin,                          
                              const char* regex_group_end,
                              std::vector<S> state_start_set,
                              S write_state,
                              ExpressionGroupQuantification grp_quantification);
  inline S transition(S input_state, char input_ch); 
  inline T stateType(S state);
  inline void writeStateType(S state, T type);

  std::vector<S> epsilonSearch(std::vector<S>& base_state); 

  static const S begin_state;
  static const S garbage_state;

private:
  inline void writeTransition(S out_state, S in_state, char in_ch);

  void addEpsilonTransition(S out_state, S in_state);
  
  S makeState();

  void grow();
  void writeUnusedDefaultTransitions();

  std::vector<S>* epsilon_transitions;
};

//NFA TEMPLATE DEFINTIONS

// Note: Since FiniteAutomata<S,T,a_size> is a nondependent class.
//       Derived classes must explicitly refer to FiniteAutomata's
//       data fields with the a dependent scope (e.g. this->...)
// --->  "https://isocpp.org/wiki/faq/templates#nondependent-name-lookup-members 

template <typename S, typename T, size_t a_size>
NFA<S, T, a_size>::NFA() : FiniteAutomata<S, T, a_size> {nullptr, nullptr, 2, 10} {
  this->transition_table =
    static_cast<S(*)[a_size]>(operator new(this->num_states_max * sizeof(*this->transition_table)));
  memset(this->transition_table + garbage_state, 0, sizeof(*this->transition_table)); 
  memset(this->transition_table + begin_state,   0, sizeof(*this->transition_table)); 

  this->accept_states =
    static_cast<T*>(operator new(this->num_states_max * sizeof(*this->accept_states)));
  this->accept_states[garbage_state] = this->accept_states[begin_state] = 0;  
  
  this->epsilon_transitions = 
    static_cast<std::vector<S>*>(operator new(this->num_states_max * sizeof(*this->epsilon_transitions)));    
   
  new (this->epsilon_transitions + garbage_state) std::vector<S>();
  new (this->epsilon_transitions + begin_state)   std::vector<S>();

  writeUnusedDefaultTransitions();
}

template <typename S, typename T, size_t a_size>
NFA<S, T, a_size>::~NFA() {
  operator delete(this->transition_table);
  operator delete(this->accept_states);
  
  for (unsigned int i = 0; i < this->num_states; ++i) {
    this->epsilon_transitions[i].~vector();
  }
  operator delete(this->epsilon_transitions);
}

template <typename S, typename T, size_t a_size>
const S NFA<S, T, a_size>::garbage_state = std::numeric_limits<S>::min();

template <typename S, typename T, size_t a_size>
const S NFA<S, T, a_size>::begin_state = NFA<S, T, a_size>::garbage_state + 1;

template <typename S, typename T, size_t a_size>
S
NFA<S, T, a_size>::transition(S in_state, char in_ch) {
  return this->transition_table[in_state][in_ch];  
}

template <typename S, typename T, size_t a_size>
void
NFA<S, T, a_size>::writeTransition(S out_state, S in_state, char in_ch) {
  this->transition_table[in_state][in_ch] = out_state;
}

template <typename S, typename T, size_t a_size>
T NFA<S, T, a_size>::stateType(S state) {
  return this->accept_states[state];
}

template <typename S, typename T, size_t a_size>
void
NFA<S, T, a_size>::writeStateType(S state, T type) {
  this->accept_states[state] = type;
}

template <typename S, typename T, size_t a_size>
void
NFA<S, T, a_size>::addEpsilonTransition(S out_state, S in_state) {
  this->epsilon_transitions[in_state].push_back(out_state);
}

template <typename S, typename T, size_t a_size>
S
NFA<S, T, a_size>::makeState() {
  if (this->num_states >= this->num_states_max) {
    grow();
  }
  new (this->epsilon_transitions + this->num_states) std::vector<S>();
  return this->num_states++;
}

template <typename S, typename T, size_t a_size>
void
NFA<S, T, a_size>::grow() {
  this->num_states_max <<= 1;

  S (*new_transition_table)[a_size] =
    static_cast<S(*)[a_size]>(operator new(this->num_states_max * sizeof(*new_transition_table)));

  T* new_accept_states =
    static_cast<T*>(operator new(this->num_states_max * sizeof(*new_accept_states)));

  std::vector<S>* new_epsilon_transitions =
    static_cast<std::vector<S>*>(operator new(this->num_states_max * sizeof(*new_epsilon_transitions)));
  
  memcpy(new_transition_table, this->transition_table, sizeof(*this->transition_table) * this->num_states);
  memcpy(new_accept_states, this->accept_states, sizeof(*this->accept_states) * this->num_states);
  memcpy(new_epsilon_transitions, epsilon_transitions, sizeof(*epsilon_transitions) * this->num_states);

  operator delete(this->transition_table);
  operator delete(this->accept_states);
  operator delete(this->epsilon_transitions);
  
  this->transition_table = new_transition_table;
  this->accept_states = new_accept_states;
  this->epsilon_transitions = new_epsilon_transitions;

  writeUnusedDefaultTransitions();
}

template <typename S, typename T, size_t a_size>
void
NFA<S, T, a_size>::writeUnusedDefaultTransitions() {
  memset(this->transition_table + this->num_states,
         0,
         (this->num_states_max - this->num_states) * sizeof(*this->transition_table));

  memset(this->accept_states + this->num_states,
         0,
         (this->num_states_max - this->num_states) * sizeof(*this->accept_states));
}

template <typename S, typename T, size_t a_size>
std::vector<S>
NFA<S, T, a_size>::addExprGroup(const char* regex_group_begin,
                  const char* regex_group_end,
                  std::vector<S> start_state_set,
                  S start_write_state, // a single state, led by epsilon branches on collisions
                  ExpressionGroupQuantification grp_quantification) {
  //  [regex_begin ; regex_end[ is a complete sub expression to be simulated into start_state_set
  //  The expression can contain children sub expression(s). Groups are seperated from
  //  their qualifier (e.g. '+', '*', {n,m})
  //
  //    e.g.
  //    "abc(dfg)*((hjk)+)|(qw)" contains sub expressions (dfg)*, ((hjk)+|(wq))
  //
  //  Recursively find all sub-expressions with a depth-first approach.

  std::vector<S> current_state_set;
  S              current_governing_state = garbage_state;

  std::vector<S> next_state_set = start_state_set;
  
  S write_state = start_write_state;
  S intermediate_state = garbage_state;
  // The state set where the cycle begins, for e.g. *, +, or {n,m}
  std::vector<S> group_final_state_set;

  bool transition_buffer[a_size];
  bool last_group_in_expr;
  bool non_qualified_group;

  //TODO: Figure out correct loop patterns. for {0, ~} - {5, 7} etc
  for (int occurrence_count = 0;

       (occurrence_count < grp_quantification.min_occurrences ||
       occurrence_count < grp_quantification.max_occurrences) ||
       (occurrence_count == grp_quantification.min_occurrences &&
       grp_quantification.max_occurrences == grp_quantification.INFINITE_OCCURRENCES);

       ++occurrence_count) {

    if (intermediate_state != garbage_state) {
      write_state = intermediate_state;
    }
    intermediate_state = garbage_state;

    last_group_in_expr = false;

    if (occurrence_count == grp_quantification.min_occurrences &&
		grp_quantification.max_occurrences == grp_quantification.INFINITE_OCCURRENCES) {
      // For '+', '*' and '{n,} expression type
      // The current state set will be used for cycle connectivity
      intermediate_state = write_state;
    } else if (occurrence_count >= grp_quantification.min_occurrences) {
      group_final_state_set.push_back(write_state);
    }
    
    for (const char* regex_itr = regex_group_begin;
         regex_itr != regex_group_end; // regex_itr incremented in loop by various rule
         ) {

      memset(transition_buffer, 0, sizeof(transition_buffer));
      non_qualified_group = false;

      current_state_set = std::move(next_state_set);
      next_state_set = std::vector<S>();

      const char* subexpr_group_begin = nullptr;
      const char* subexpr_group_end   = nullptr;


      switch (*regex_itr) {
        
        case '\\': {
          // we escape the next metacharacter so just walk over it by one
          // and interpret it as a char value
          subexpr_group_begin = regex_itr;
          ++regex_itr;
          transition_buffer[*regex_itr] = true;
          regex_itr = subexpr_group_end = regex_itr + 1;
          break;
        }
        case '(': {
          // Treat the contents of (...) as a subexpression and resolve the outcome
          // depending on whether there was a (...)*, (...)+ or just (...)
          
          // find matching ')' on same depth (nested) level.  
          ++regex_itr;
          subexpr_group_begin = regex_itr;

          for (int depth = 1;
               depth != 0;
               ++regex_itr) {

            if (*regex_itr == '\0') {
              std::cerr << "Bad regular expression: no matching ')' for '('" << std::endl;
            }
            switch (*regex_itr) {
              case '\0': {
                std::cerr << "Bad regular expression: no matching ')' for '('" << std::endl;
                break;
              }
              case '(': {
                ++depth;
                break;
              }
              case ')': {
                --depth;
                break;
              }
            }
          }
          non_qualified_group = true;
          subexpr_group_end = regex_itr - 1;
          break;
        }
        case ')': {
          std::cerr << "Bad regular expression: no matching '(' for ')'" << std::endl;
          break;
        }
        case '|': {
          next_state_set = start_state_set;
          write_state = start_write_state;
          last_group_in_expr = false;
          ++regex_itr;
          continue;
        }
        case '[': {
          // only need one state until we find a matchin ']' 
          // move the iterator to that position and continue
          subexpr_group_begin = regex_itr;
          const char* look_ahead = regex_itr + 1;
          const char* first_char = look_ahead;
          
          bool flip_transition = false;
          if (*look_ahead == '^') {
            flip_transition = true;
            ++look_ahead;
            memset(transition_buffer, true, sizeof(transition_buffer));
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
                } // TODO: off by one?
                if (flip_transition) {
                  memset(transition_buffer + first_val, false, end_val - first_val);
                } else {
                  memset(transition_buffer + first_val, true, end_val - first_val);
                }
                break;
              }
              default: {
                first_val = *look_ahead;
                if (flip_transition) {
                  transition_buffer[first_val] = false;
                } else {
                  transition_buffer[first_val] = true;
                }
              }
            }
            ++look_ahead;
          }
          subexpr_group_end = regex_itr = look_ahead + 1;
          break;
        }
        case '.': {
          // can match any character
          subexpr_group_begin = regex_itr;
          subexpr_group_end = ++regex_itr;
          memset(transition_buffer, true, sizeof(transition_buffer));
          break;
        }
        default: {
          subexpr_group_begin = regex_itr;
          transition_buffer[*regex_itr] = true;
          subexpr_group_end = ++regex_itr;
          break;
        }
      }

      ExpressionGroupQuantification quant;
      int quantification_length = quant.quantifyOnString(regex_itr);

      // Recursive call on a grouped subexpression
      if ((quantification_length != 0 && regex_itr != regex_group_end) || 
          non_qualified_group) {
        next_state_set = addExprGroup(subexpr_group_begin,
                                      subexpr_group_end,
                                      current_state_set,
                                      write_state,
                                      quant);

        regex_itr += quantification_length;
        write_state = next_state_set[0];
      } else { // Otherwise do ordinary state transition writes

        bool reusable_existing_state = true;
        for (unsigned char i = 0;
        i < a_size;
          ++i) {

          if (transition_buffer[i] && transition(write_state, i) != garbage_state) {
            reusable_existing_state = false;
            break;
          }
        }

        if (!reusable_existing_state) {
          S epsilon_state = makeState();
          addEpsilonTransition(epsilon_state, write_state);
          write_state = epsilon_state;
        }

        S target_state = makeState();

        // write_state is now collision free and transitions can be written to target_state
        for (unsigned char i = 0;
        i < a_size;
          ++i) {

          if (transition_buffer[i]) {
            writeTransition(target_state, write_state, i);
          }
        }

        next_state_set.push_back(target_state);
        write_state = target_state;
      }

      if (regex_itr == regex_group_end || *regex_itr == '|') {
        last_group_in_expr = true;
      }

      if (last_group_in_expr) {
        if (occurrence_count == grp_quantification.min_occurrences &&
          grp_quantification.max_occurrences == grp_quantification.INFINITE_OCCURRENCES) {

          // add epsilon transition to the previous cycle write state
          addEpsilonTransition(intermediate_state, write_state);
          group_final_state_set.push_back(intermediate_state);

        } else {
          if (intermediate_state == garbage_state) {
            intermediate_state = makeState();
          }
          
          addEpsilonTransition(intermediate_state, write_state);
        }
      }
    }
  }
  group_final_state_set.push_back(intermediate_state);

  for (auto state : next_state_set) {
	  group_final_state_set.push_back(state);
  }
  return group_final_state_set;
}

// Only peeks in one epsilon transition depth. Deeper transitions should never occurr.
template <typename S, typename T, size_t a_size>
std::vector<S>
NFA<S, T, a_size>::epsilonSearch(std::vector<S>& base_states) {
  std::vector<S> expanded_states; 

  
  for (auto state : base_states) {
    expanded_states.push_back(state);
  }
  for (auto state : base_states) {
    /*
    Why is dereferencing this->epsilon_transitions[state] through foreach an error...?!
    for (auto epsilson_state : this->epsilon_transitions[state]) {
      expanded_states.push_back(epsilson_state);
    }
    */
    const size_t n = this->epsilon_transitions[state].size();
    for (size_t i = 0; i < n; ++i) {
      S transition = this->epsilon_transitions[state][i];
      if (std::find(expanded_states.begin(), expanded_states.end(), transition) ==
          expanded_states.end()) {
        expanded_states.push_back(transition);
      }
    }
  }
  return expanded_states;
}

#endif // NFA_H_

