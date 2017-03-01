
#include <vector>

#include "finite_automata.h"

// Expression group quantification:
//
// "*" : zero or more occurrences
// "+" : one or more occurrences 
// "?" : zero or one occurences 
// "{n}" : exactly n occurrences
// "{n,}" : n or more occurrences
// "{n,m}": between n and m occurences (inclusive)
// "{,m}" : between zero and m occurences
struct ExpressionGroupQuantification {
  ExpressionGroupQuantification(int min = 1, int max = 1);
  ExpressionGroupQuantification(const char* quantifier_begin);
  
  int quantifyOnString(const char* quantification_begin);

  int min_occurrences;
  int max_occurrences;

  const int INFINITE_OCCURRENCES = -1;
};

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

template <typename S, typename T, size_t a_size>
NFA<S, T, a_size>::NFA() : FiniteAutomata {nullptr, nullptr, 2, 10} { // 1,10
  transition_table =
    static_cast<S(*)[a_size]>(operator new(num_states_max * sizeof(*transition_table)));
  memset(transition_table + garbage_state, 0, sizeof(*transition_table)); 
  memset(transition_table + begin_state,   0, sizeof(*transition_table)); 

  accept_states =
    static_cast<T*>(operator new(num_states_max * sizeof(*accept_states)));
  accept_states[garbage_state] = accept_states[begin_state] = 0;  
  
  epsilon_transitions = 
    static_cast<std::vector<S>*>(operator new(num_states_max * sizeof(*epsilon_transitions)));    
   
  new (epsilon_transitions + garbage_state) std::vector<S>();
  new (epsilon_transitions + begin_state)   std::vector<S>();

  writeUnusedDefaultTransitions();
}

template <typename S, typename T, size_t a_size>
NFA<S, T, a_size>::~NFA() {
  operator delete(transition_table);
  operator delete(accept_states);
  
  for (unsigned int i = 0; i < num_states; ++i) {
    epsilon_transitions[i].~vector<S>();
  }
  operator delete(epsilon_transitions);
}

template <typename S, typename T, size_t a_size>
const S NFA<S, T, a_size>::garbage_state = std::numeric_limits<S>::min();

template <typename S, typename T, size_t a_size>
const S NFA<S, T, a_size>::begin_state = NFA<S, T, a_size>::garbage_state + 1;

template <typename S, typename T, size_t a_size>
S
NFA<S, T, a_size>::transition(S in_state, char in_ch) {
  return transition_table[in_state][in_ch];  
}

template <typename S, typename T, size_t a_size>
void
NFA<S, T, a_size>::writeTransition(S out_state, S in_state, char in_ch) {
  transition_table[in_state][in_ch] = out_state;
}

template <typename S, typename T, size_t a_size>
T NFA<S, T, a_size>::stateType(S state) {
  return accept_states[state];
}

template <typename S, typename T, size_t a_size>
void
NFA<S, T, a_size>::writeStateType(S state, T type) {
  accept_states[state] = type;
}

template <typename S, typename T, size_t a_size>
void
NFA<S, T, a_size>::addEpsilonTransition(S out_state, S in_state) {
  epsilon_transitions[in_state].push_back(out_state);
}

template <typename S, typename T, size_t a_size>
S
NFA<S, T, a_size>::makeState() {
  if (num_states >= num_states_max) {
    grow();
  }
  new (epsilon_transitions + num_states) std::vector<S>();
  return num_states++;
}

template <typename S, typename T, size_t a_size>
void
NFA<S, T, a_size>::grow() {
    const size_t new_max = 2 * num_states_max;

    S (*new_transition_table)[a_size] =
      static_cast<S(*)[a_size]>(operator new(new_max * sizeof(*new_transition_table)));

    T* new_accept_states =
      static_cast<T*>(operator new(new_max * sizeof(*new_accept_states)));

    std::vector<S>* new_epsilon_transitions =
      static_cast<std::vector<S>*>(operator new(new_max * sizeof(*new_epsilon_transitions)));
    
    memcpy(new_transition_table, transition_table, sizeof(*transition_table) * num_states);
    memcpy(new_accept_states, accept_states, sizeof(*transition_table) * num_states);
    memcpy(new_epsilon_transitions, epsilon_transitions, sizeof(*epsilon_transitions) * num_states);

    operator delete(transition_table);
    operator delete(accept_states);
    operator delete(epsilon_transitions);
    
    transition_table = new_transition_table;
    accept_states = new_accept_states;
    epsilon_transitions = new_epsilon_transitions;

    writeUnusedDefaultTransitions();
}

template <typename S, typename T, size_t a_size>
void
NFA<S, T, a_size>::writeUnusedDefaultTransitions() {
  memset(transition_table + num_states,
         0,
         (num_states_max - num_states) * sizeof(*transition_table));

  memset(accept_states + num_states,
         0,
         (num_states_max - num_states) * sizeof(*accept_states));
}

template <typename S, typename T, size_t a_size>
std::vector<S>
NFA<S, T, a_size>::addExprGroup(const char* regex_group_begin,
                  const char* regex_group_end,
                  std::vector<S> start_state_set,
                  S write_state, // a single state, led by epsilon branches on collisions
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

  // The state set where the cycle begins, for e.g. *, +, or {n,m}
  S              cycle_state;
  std::vector<S> cycle_connectivity_state_set;
  std::vector<S> group_final_state_set;

  bool last_group_in_expr;

  bool transition_buffer[a_size];
  //TODO: Figure out correct loop patterns. for {0, ~} - {5, 7} etc
  for (int occurrence_count = 0;

       occurrence_count < grp_quantification.min_occurrences ||
       occurrence_count < grp_quantification.max_occurrences ||
       grp_quantification.max_occurrences == grp_quantification.INFINITE_OCCURRENCES;

       ++occurrence_count) {

    if (occurrence_count == grp_quantification.min_occurrences &&
		grp_quantification.max_occurrences == grp_quantification.INFINITE_OCCURRENCES) {
      // For '+', '*' and '{n,} expression type
      // The current state set will be used for cycle connectivity
      cycle_state = write_state;
      for (auto accept_state : next_state_set) {
        group_final_state_set.push_back(accept_state);
      }
    }

    for (const char* regex_itr = regex_group_begin;
         regex_itr != regex_group_end;
         ++regex_itr) {

      memset(transition_buffer, 0, sizeof(transition_buffer));
	  // epsilon branch before moving?
      current_state_set = std::move(next_state_set);
      next_state_set = std::vector<S>();

      if (regex_itr + 1 == regex_group_end) {
        last_group_in_expr = true;
      }

      switch (*regex_itr) {
        
        case '\\': {
          // we escape the next metacharacter so just walk over it by one
          // and interpret it as a char value
          ++regex_itr;
          transition_buffer[*regex_itr] = true;
          break;
        }
        case '(': {
          // Treat the contents of (...) as a subexpression and resolve the outcome
          // depending on whether there was a (...)*, (...)+ or just (...)
          
          // find matching ')' on same depth (nested) level.  
          ++regex_itr;
          const char* sub_expr_begin = regex_itr;

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
          --regex_itr;

          ExpressionGroupQuantification quantification;
          int quantification_length = quantification.quantifyOnString(regex_itr + 1);

          next_state_set = addExprGroup(sub_expr_begin,
                                        regex_itr,
                                        current_state_set,
                                        write_state,
                                        quantification);

          regex_itr += quantification_length;
          write_state = next_state_set[0];
          continue;
        }
        case ')': {
          std::cerr << "Bad regular expression: no matching '(' for ')'" << std::endl;
          break;
        }
        case '|': {
          //TODO: Save current final states, and continue from here with start state
          for (S final_state : current_state_set) {
            group_final_state_set.push_back(final_state);
          }
          next_state_set = start_state_set;

          continue;
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
            memset(transition_buffer, true, *transition_buffer);
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
          regex_itr = look_ahead;
          break;
        }
        case '.': {
          // can match any character
          ExpressionGroupQuantification quantification_check;
          int quantification_length = quantification_check.quantifyOnString(regex_itr + 1);
          if (quantification_length == 0 || regex_itr + 1 == regex_group_end) {
            memset(transition_buffer, true, sizeof(transition_buffer));
            break;
          } else {
            next_state_set = addExprGroup(regex_itr,
                                          regex_itr + 1,
                                          current_state_set,
                                          write_state,
                                          quantification_check);
            regex_itr += quantification_length;
            write_state = next_state_set[0];
            continue;
          }
        }
        default: {
          // Normal character. Look ahead character quantification (e.g. a*, a+ ,a?, a{n,m}
          
          ExpressionGroupQuantification quantification_check;
          int quantification_length = quantification_check.quantifyOnString(regex_itr + 1);
          if (quantification_length == 0 || regex_itr + 1 == regex_group_end) {
            transition_buffer[*regex_itr] = true;
            break;
          } else {
            next_state_set = addExprGroup(regex_itr,
                                          regex_itr + 1,
                                          current_state_set,
                                          write_state,
                                          quantification_check);
            regex_itr += quantification_length;
            write_state = next_state_set[0];
            continue;
          }
        }
      }

      // State transitions writes
	  
      bool reusable_existing_state = true;
      S first_peek = garbage_state;
      for (unsigned char i = 0;
         i < a_size;
         ++i) { 
        if (transition_buffer[i]) {
          if (first_peek == garbage_state) {
            first_peek = transition(write_state, i);
          } else {
            if (first_peek != transition(write_state, i)) {
              reusable_existing_state = false;
              break;
            }
          }
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

      if (last_group_in_expr &&
          occurrence_count == grp_quantification.min_occurrences &&
          grp_quantification.max_occurrences == grp_quantification.INFINITE_OCCURRENCES)  {
        // add epsilon transition to the previous cycle write state
        addEpsilonTransition(cycle_state, write_state);
        next_state_set.push_back(cycle_state);
        grp_quantification.max_occurrences = occurrence_count; // to break out of the outer loop
      } else if (last_group_in_expr &&
          occurrence_count >= grp_quantification.min_occurrences) {
        // next_state_set are accept states
        for (auto accept_state : next_state_set) {
          group_final_state_set.push_back(accept_state);
        }
      }
    }
  }

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
    for (auto epsilson_state : epsilon_transitions[state]) {
      expanded_states.push_back(epsilson_state);
    }
    expanded_states.push_back(state);
  }
  return expanded_states;
}

