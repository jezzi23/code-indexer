
#include "lexer.h"

#include <regex>
#include <cstring>
#include <vector>
#include <iostream>
#include <cassert>
#include <stdio.h>

#include "file_mapped_io.h"

internal_ bool
hasFileExtension(const char* filename, const char* extension);

internal_ bool
isMeaninglessChar(LexingIterator& lexer);

Token::Token() {

}

Token::Token(LexingIterator lex_itr, TokenIdentifier identifier) :
    index(lex_itr.token_begin - lex_itr.begin),
    length(lex_itr.itr - lex_itr.token_begin),
    id(identifier),
    line_count(lex_itr.token_line),
    column_count(lex_itr.token_column) {
}

// Temporary check 
internal_ bool
hasFileExtension(const char* filename, const char* extension) {
  for (;;) {
    if (*filename == '\0') {
      return false;

    } else if (*filename == '.') {
      ++filename;
      break;

    } else {
      ++filename; 
    }
  }
  return !strcmp(filename, extension);
}

inline bool
isMeaninglessChar(LexingIterator& lexer) {
  if (*lexer.itr == '\t' ||
      *lexer.itr == '\r' ||
      *lexer.itr == ' ') {
    return true;
  } else if (*lexer.itr == '\n') {
    ++lexer.line_count;
    lexer.last_line_begin = lexer.itr;
    return true;
  } else {
    return false;
  }
}

inline static bool
nextNCharsExist(LexingIterator lex_itr, int n) {
  return n <= lex_itr.end - lex_itr.itr; 
}

std::vector<Token>
tokenize(const char* filename) {
  if (!hasFileExtension(filename, "c")) {
    std::cout << "Loaded unsupported file format: " << filename << std::endl; 
  }

  FileMapper filemap(filename);
  u32 file_size = static_cast<u32>(filemap.getFileSize());
  
  const char* file_begin = static_cast<const char*>(filemap.map(0, file_size));
  const char* file_end   = file_begin + file_size;

  LexingIterator lexer = {file_begin, file_begin, file_end,
                          file_begin - 1, 1,
                          file_begin, 1, 1};

  std::vector<Token> out_tokens;

  for (; lexer.itr != lexer.end; ++lexer.itr) {

    if (isMeaninglessChar(lexer)) continue;

    lexer.token_begin = lexer.itr; 
    lexer.token_line = lexer.line_count;
    lexer.token_column = lexer.itr - lexer.last_line_begin;

    switch (*lexer.itr) {
      case '(': {
        out_tokens.push_back(Token(lexer,
                                   TokenIdentifier::OPEN_PARANTHESIS));
        continue;
      }
      case ')': {
        out_tokens.push_back(Token(lexer,
                                   TokenIdentifier::CLOSE_PARANTHESIS));
        continue;
      }
      case '[': {
        out_tokens.push_back(Token(lexer,
                                   TokenIdentifier::OPEN_SQUARE_BRACKET));
        continue;
      }
      case ']': {
        out_tokens.push_back(Token(lexer,
                                   TokenIdentifier::CLOSE_SQUARE_BRACKET));
        continue;
      }
      case '{': {
        out_tokens.push_back(Token(lexer,
                                   TokenIdentifier::OPEN_BRACKET));
        continue;
      }
      case '}': {
        out_tokens.push_back(Token(lexer,
                                   TokenIdentifier::CLOSE_BRACKET));
        continue;
      }
    }

    // Detect coments 
    if (nextNCharsExist(lexer, 2) && lexer.itr[0] == '/') {
      if (lexer.itr[1] == '*') {
        ++lexer.itr;
        // C-style comment
        while (++lexer.itr != lexer.end) {
          if (isMeaninglessChar(lexer)) continue;
          if (lexer.itr[0] == '*' && nextNCharsExist(lexer, 2) && lexer.itr[1] == '/') {
            ++lexer.itr;
            out_tokens.push_back(Token(lexer,
                                       TokenIdentifier::COMMENT));
            break;
          }
        }
        continue;
      } else if (lexer.itr[1] == '/') {
        // C++ style comment
        while (++lexer.itr != lexer.end) {
          if (*lexer.itr == '\n') {
            out_tokens.push_back(Token(lexer,
                                       TokenIdentifier::COMMENT)); 
            break;
          }
        }
        continue;
      }
    }
     
    // Detect strings
    if (*lexer.itr == '"') {
      // Find a matching ", except for an escaped "
      while (++lexer.itr != lexer.end) {
        if (lexer.itr[0] == '"' && !(lexer.itr[-1] == '\\')) {
          out_tokens.push_back(Token(lexer,
                                     TokenIdentifier::LITERAL_STRING)); 
          break;
        }
      }
      continue;
    }
    
    // Detect a char
    if (*lexer.itr == '\'') {
      while (++lexer.itr != lexer.end) {
        // Find a matchin ', except for an escaped '
        if (lexer.itr[0] == '\'' && !(lexer.itr[-1] == '\\')) {
          out_tokens.push_back(Token(lexer,
                                     TokenIdentifier::LITERAL_CHAR)); 
          break;
        }
      }
      continue;
    }

    // Detect number types
    if ((*lexer.itr >= '0' && *lexer.itr <= '9') || *lexer.itr == '.') {
      bool is_float = false; 
      if (*lexer.itr == '.') is_float = true;

      while (++lexer.itr != lexer.end &&
             (*lexer.itr >= '0' && *lexer.itr <= '9') ||
             *lexer.itr == '.') {
         
      }
      
      // Number ended
      out_tokens.push_back(
          Token(lexer,
                is_float ? TokenIdentifier::LITERAL_FLOAT : TokenIdentifier::LITERAL_INT)); 
      continue;
    }

    // Otherwise assume its some kind of word
    do {
      ++lexer.itr;

    } while (lexer.itr  != lexer.end &&
             *lexer.itr != '\n'      &&
             *lexer.itr != '\t'      &&
             *lexer.itr != '\r'      &&
             *lexer.itr != ' '       &&
             *lexer.itr != '('       &&
             *lexer.itr != ')'       &&
             *lexer.itr != ']'       &&
             *lexer.itr != '{'       &&
             *lexer.itr != '}'); 

    --lexer.itr; // back off by one since spaces, newlines etc... is not a part of the word
    out_tokens.push_back(Token(lexer,
                               TokenIdentifier::NAME)); 
  }

  out_tokens.push_back(Token(lexer, TokenIdentifier::END_OF_FILE)); 
  for (auto itr = out_tokens.begin(); itr != out_tokens.end(); ++itr) {
    printf("Token nr %d.\nIndex: %d\n Length: %d\n id: %d\n line_count: %d\n column_count: %d\n Contents: \n%.*s\n\n",
           (int)(itr - out_tokens.begin()),
           (int)itr->index,
           (int)itr->length,
           (int)itr->id,
           (int)itr->line_count,
           (int)itr->column_count,
           (int)itr->length,
           lexer.begin + itr->index);
  }
  return out_tokens;
}

Lexer::Lexer(const char* regex, const char* lexing_data, unsigned int input_length) :
    lexing_data{lexing_data, lexing_data, lexing_data + input_length,
                lexing_data - 1, 1,
                lexing_data , 1, 1} {
  buildDFA(regex);
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
    if (state != 0) {
      int breakme = 5;
    }
    if (isFinishState(state)) {
      return Token(lexing_data, TokenIdentifier::NAME);
    }
  }
  return Token(lexing_data, TokenIdentifier::END_OF_FILE);
}

inline bool
Lexer::isFinishState(unsigned int state) {
  return dfa.accept_states.find(state) != dfa.accept_states.end();
}

void
Lexer::reset() {
  lexing_data.itr = lexing_data.begin;

  lexing_data.last_line_begin = lexing_data.begin - 1;
  lexing_data.token_begin = lexing_data.begin;

  lexing_data.line_count = 1;
}

// TODO: How to deal with |, e.g. (abc)* | bca | cb+
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
      }
      case '(': {
        // a group... continue to next character
        continue;
      }
      case ')': {
        // a closing group... continue to next character
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

void Lexer::buildDFA(const char* regex) {
  
  dfa.num_states = estimateNumStates(regex);
  const unsigned int dfa_width = dfa.alphabet_size;
  dfa.states = static_cast<unsigned int*>(
      operator new(dfa.num_states * sizeof(*dfa.states) * dfa_width));
  memset(dfa.states, 0, dfa.num_states * sizeof(*dfa.states) * dfa_width);
  // dfa.states operates as follows
  // dfa.states[STATE_NUMBER * dfa_width + INPUT_CHARACTER] = NEW_STATE_NUMBER

  std::vector<char> next_transition;
  next_transition.reserve(128);

  unsigned int current_state;
  unsigned int next_state = 0;

  for (const char* itr = regex; *itr != '\0'; ++itr) {
    current_state = next_state;

    assert(current_state != dfa.num_states);

    next_transition.clear();

    switch (*itr) {
      // deal with special metacharacters 
      case '[': {
        // only need one state until we find a matchin ']' 
        // move the iterator to that position and continue
        const char* look_ahead = ++itr;
        const char* first_char = itr;
        bool flip_transition = false;
        if (*first_char == '^') {
          flip_transition = true;
          ++look_ahead;
          for (unsigned char i = 0; i < 128; ++i) {
            next_transition.push_back(i);
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
                  next_transition.erase(std::lower_bound(next_transition.begin(),
                                                         next_transition.end(),
                                                         val));
                } else {
                  next_transition.push_back(val);
                }
              }
              break;
            }
            default: {
              first_val = *look_ahead;
              if (flip_transition) {
                next_transition.erase(std::lower_bound(next_transition.begin(),
                                                       next_transition.end(),
                                                       first_val));
              } else {
                next_transition.push_back(first_val);
              }
            }
          }
          ++look_ahead;
        }
        itr = look_ahead;
        break;
      }
      case '\\': {
        // we escape the next metacharacter so just walk over it by one
        ++itr;
        // don't break, instead fall down to default
      }
      case '(': {
        // a group... continue to next character
        continue;
      }
      case ')': {
        // a closing group... continue to next character
      }
      default: {
        next_transition.push_back(*itr);
        // normal character
        break;
      }
    }
    // we now have collected new state transitions in next_transition
    // look for '*', '+' etc metacharacters to determine how to write the trasnitions 
    const char* look_ahead = itr + 1;
    switch (*look_ahead) {
      case '*': {
        for (auto char_transition : next_transition) {
          if (dfa.states[current_state * dfa_width + char_transition] == dfa.begin_state) {
            dfa.states[current_state * dfa_width + char_transition] = current_state;
          }
        }
        itr = look_ahead;
        break;
      }
      case '+':  {
        next_state = current_state + 1;
        for (auto char_transition : next_transition) {
          if (dfa.states[current_state * dfa_width + char_transition] == dfa.begin_state) {
            dfa.states[current_state * dfa_width + char_transition] = next_state;
            dfa.states[next_state * dfa_width + char_transition] = next_state;
            
          }
        }
        itr = look_ahead;
        break;
      }
      default: {
        next_state = current_state + 1;
        for (auto char_transition : next_transition) {
          if (dfa.states[current_state * dfa_width + char_transition] == dfa.begin_state) {
            dfa.states[current_state * dfa_width + char_transition] = next_state;
          }
        }
        break;
      }
    }
  }
  dfa.accept_states.insert(std::pair<unsigned int, unsigned int>(next_state, 0));
}

