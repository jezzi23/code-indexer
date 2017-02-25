
#include <limits>

// FiniteAutomata is a finite state machine that knows nothing about how it's built or used.
// A lexical analyzer or a regex engine can wrap FiniteAutomata as a DFA, NFA or whatever.
template <typename S = unsigned int,   // S is the state type

          typename T = int,            // T is the acceptance type. Allows lexer to match
                                       // various token with a unique id e.g. int.
                                       // For a normal regex engine, a bool type is sufficient
                                       // and could be packed to utilize 8 bits in each byte
                               
          size_t a_size 1 << 7>        // a_size is the size of the alphabet. Defaults to ASCII
struct FiniteAutomata {
  
  static const S begin_state;
  static const S garbage_state;

  S* transition_table[a_size];
  T* accept_states;
  
  size_t num_states;
  size_t num_states_max;
};

//FiniteAutomata TEMPLATE DEFINTIONS
template <typename S, typename T, size_t a_size>
static const S FiniteAutomata::begin_state = FiniteAutomata::garbage_state + 1;

template <typename S, typename T, size_t a_size>
static const S FiniteAutomata::garbage_state = std::numeric_limits<S>::min();

