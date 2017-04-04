
#include <limits>

template <typename S, typename T, size_t a_size>
class DFA : private FiniteAutomata<S, T, a_size> {
public: 
  DFA();
  ~DFA();

  std::vector<S> addRule(const char* expr, T token_id);

private:
  inline S transition(S input_state, char input_ch); 
  inline void writeTransition(S out_state, S in_state, char in_ch);
  inline void copyTransitions(S dest, S src);

  inline T stateType(S state);
  inline void writeStateType(S state, T type);

  
  S makeState();

  void grow();
};

