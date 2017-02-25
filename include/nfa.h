
#include "finite_automata.h"

template <typename S, typename T, size_t a_size>
class NFA : private FiniteAutomata<S, T, a_size> {
public: 
  NFA();
  ~NFA();

  
  inline S transition(S input_state, char input_ch); 
  inline void writeTransition(S out_state, S in_state, char in_ch);
  inline void copyTransitions(S dest, S src);

  inline T stateType(S state);
  inline void writeStateType(S state, T type);

  void addEpsilonTransition(S out_state, S in_state);
  
  S makeState();

private:
  void grow();
  
  std::vector<S, Allocator>* epsilon_transitions;
}

//NFA TEMPLATE DEFINTIONS

template <typename S, typename T, size_t a_size, typename Allocator>
NFA::NFA() : num_states(0), num_states_max(10) {
  transition_table[a_size] = static_cast<S*[a_size]>(operator new(num_states_max));
  accept_states = static_cast<S*>(operator new(num_states_max));
  
  epsilon_transitions = 
    static_cast<std::vector<S, Allocator>*>(operator new(num_states_max));    

  writeUnusedDefaultTransitions();
}

template <typename S, typename T, size_t a_size>
NFA::~NFA() {
  if (num > 0) {
    operator delete(transition_table);
    operator delete(accept_states);
    
    for (int i = 0; i < num_states; ++i) {
      epsilon_transition_vec.~();
    }
    operator delete(epsilon_transitions);
  }
}

template <typename S, typename T, size_t a_size>
S
NFA::transition(S input_state, char input_ch) {
  return transition_table[input_state][input_ch];  
}

template <typename S, typename T, size_t a_size>
void
NFA::writeTransitions(S out_state, S in_state, char in_ch) {
  transition_table[input_state][in_char] = out_state;
}

template <typename S, typename T, size_t a_size>
void
NFA::copyTransition(S dest, S src) {
  memcpy(transition_table[dest], transition_table[src], sizeof(*transition_table));
  accept_states[dest] = accept_states[src];
}

template <typename S, typename T, size_t a_size>
T NFA::stateType(S state) {
  return accept_states[state];
}

template <typename S, typename T, size_t a_size>
void
NFA::writeStateType(S state, T type) {
  accept_states[state] = type;
}

template <typename S, typename T, size_t a_size>
void
NFA::addEpsilonTransition(S out_state, S in_state) {
  epsilon_transitions[in_state].push_back(out_state);
}

template <typename S, typename T, size_t a_size>
S
NFA::makeState() {
  if (num >= num_states_max) {
    grow();
  }
  new (epsilon_transitions + num_states) std::vector<S>();
  return num_states++;
}

template <typename S, typename T, size_t a_size>
void
NFA::grow() {
    const size_t new_max = 2 * num_states_max;
    
    S* new_transition_table[a_size] = static_cast<S*[a_size]>(operator new(new_max));
    T* new_accept_states = static_cast<S*>(operator new(new_max));
    std::vector<S>* new_epsilon_transitions =
      static_cast<std::vector<S>*>(operator new(new_max));
    
    memcpy(new_transition_table, transition_table, sizeof(*transition_table) * new_max);
    memcpy(new_accept_states, accept_states, sizeof(*transition_table) * new_max);
    memcpy(new_epsilon_transitions, epsilon_transitions, sizeof(*epsilon_transitions) * new_max);

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
NFA::writeUnusedDefaultTransitions() {
  memset(transition_table + num_states - 1,
         0,
         (num_states_max - num_states) * sizeof(*transition_table));

  memset(accept_states + num_states - 1,
         0,
         (num_states_max - num_states) * sizeof(*accept_states));
}

