
#ifndef FINITE_AUTOMATA_H_
#define FINITE_AUTOMATA_H_

#include <cstring>
#include <limits>

// FiniteAutomata is a finite state machine that knows nothing about how it's built or used.
// A lexical analyzer or a regex engine can wrap FiniteAutomata as a DFA, NFA or whatever.
template <typename S,       // S is the state type

          typename T,       // T is the acceptance type. Allows lexer to match
                                  // various token with a unique id e.g. int.
                                  // For a normal regex engine, a bool type is sufficient
                                  // and could be packed to utilize 8 bits in each byte
                           
          size_t a_size>    // a_size is the size of the alphabet. Defaults to ASCII
struct FiniteAutomata {
  

  S (*transition_table)[a_size];
  T* accept_states;
  
  size_t num_states;
  size_t num_states_max;
};

#endif // FINITE_AUTOMATA_H_

