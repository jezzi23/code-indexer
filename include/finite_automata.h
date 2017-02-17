
#include <limits>

// FiniteAutomata is a finite state machine that knows nothing about how it's built or used.
// A lexical analyzer or a regex engine can wrap FiniteAutomata as a DFA, NFA or whatever.
template <typename S = unsigned int, // State type

          typename T = int,          // Acceptance type. Allows lexer to match
                                     // various token with e.g. int.
                                     // For a normal regex engine, a bool type is sufficient
                                     // and could be packed to utilize 8 bits in each byte
                               
          size_t a_size 1 << 7>      // The size of the alphabet. Defaults to ASCII
class FiniteAutomata {
  
public:
  FiniteAutomata();
  ~FiniteAutomata();
  
  inline S transition(S input_state, char input_ch); 
  inline void writeTransition(S out_state, S in_state, char in_ch);
  
  S makeState();
  
  static const S begin_state;
private:
  void grow();

  S* transition_table[a_size];
  T* accept_states;
  
  size_t num_states;
  size_t num_states_max;
};

//FiniteAutomata TEMPLATE DEFINTIONS
template <typename S, typename T, size_t a_size>
static const S FiniteAutomata::begin_state = std::numeric_limits<S>::min();

template <typename S, typename T, size_t a_size>
FiniteAutomata::FiniteAutomata() : num_states(0), num_states_max(0) {

}

template <typename S, typename T, size_t a_size>
FiniteAutomata::~FiniteAutomata() {
  if (num > 0) {
    operator delete(transition_table);
    operator delete(accept_states);
  }
}

template <typename S, typename T, size_t a_size>
S
FiniteAutomata::transition(S input_state, char input_ch) {
  return transition_table[input_state][input_ch];  
}

template <typename S, typename T, size_t a_size>
void
FiniteAutomata::writeTransition(writeTransition(S out_state, S in_state, char in_ch) {
  transition_table[input_state][in_char] = out_state;
}

template <typename S, typename T, size_t a_size>
S
FiniteAutomata::makeState() {
  if (num >= num_states_max) {
    grow();
  }
  
  return num_states++;
}

template <typename S, typename T, size_t a_size>
void
FiniteAutomata::grow() {
    const size_t new_max = 2 * num_states_max;
    
    S* new_transition_table[a_size] = static_cast<S*[a_size]>(new_max);
    S* new_accept_states = static_cast<S*>(new_max);
    
    memcpy(&new_transition_table,
           transition_table,
           sizeof(*transition_table) * new_max);
    memcpy(&new_accept_states,
           accept_states,
           sizeof(*transition_table) * new_max);

    operator delete(transition_table);
    operator delete(accept_states);
    
    transition_table = new_transition_table;
    accept_states = new_accept_states;
}

