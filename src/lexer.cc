
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

Token::Token(LexingIterator lex_itr, TokenIdentifier identifier, int token_length) :
    index(lex_itr.current_token_begin - lex_itr.begin),
    length(token_length),
    id(identifier),
    line_count(lex_itr.line_count),
    column_count(lex_itr.current_token_column) {
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

  return static_cast<bool>(!strcmp(filename, extension)); 
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

  LexingIterator lexer = {file_begin, file_end, file_begin,
                          file_begin - 1, file_begin,
                          1, 1};
  std::vector<Token> out_tokens;

  for (; lexer.itr != lexer.end; ++lexer.itr) {

    if (isMeaninglessChar(lexer)) continue;

    lexer.current_token_begin = lexer.itr; 
    lexer.current_token_column = lexer.itr - lexer.last_line_begin;

    switch (*lexer.itr) {
      case '(': {
        out_tokens.push_back(Token(lexer,
                                   TokenIdentifier::OPEN_PARANTHESIS,
                                   1));
        continue;
      }
      case ')': {
        out_tokens.push_back(Token(lexer,
                                   TokenIdentifier::CLOSE_PARANTHESIS,
                                   1));
        continue;
      }
      case '[': {
        out_tokens.push_back(Token(lexer,
                                   TokenIdentifier::OPEN_SQUARE_BRACKET,
                                   1));
        continue;
      }
      case ']': {
        out_tokens.push_back(Token(lexer,
                                   TokenIdentifier::CLOSE_SQUARE_BRACKET,
                                   1));
        continue;
      }
      case '{': {
        out_tokens.push_back(Token(lexer,
                                   TokenIdentifier::OPEN_BRACKET,
                                   1));
        continue;
      }
      case '}': {
        out_tokens.push_back(Token(lexer,
                                   TokenIdentifier::CLOSE_BRACKET,
                                   1));
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
                                       TokenIdentifier::COMMENT,
                                       lexer.itr - lexer.current_token_begin + 1));
            break;
          }
        }
        continue;
      } else if (lexer.itr[1] == '/') {
        // C++ style comment
        while (++lexer.itr != lexer.end) {
          if (*lexer.itr == '\n') {
            out_tokens.push_back(Token(lexer,
                                       TokenIdentifier::COMMENT,
                                       lexer.itr - lexer.current_token_begin + 1)); 
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
                                     TokenIdentifier::LITERAL_STRING,
                                     lexer.itr - lexer.current_token_begin + 1)); 
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
                                     TokenIdentifier::LITERAL_CHAR,
                                     lexer.itr - lexer.current_token_begin + 1)); 
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
                is_float ? TokenIdentifier::LITERAL_FLOAT : TokenIdentifier::LITERAL_INT,
                lexer.itr - lexer.current_token_begin + 1)); 
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
                               TokenIdentifier::NAME,
                               lexer.itr - lexer.current_token_begin + 1)); 
  }

  out_tokens.push_back(Token(lexer, TokenIdentifier::END_OF_FILE, 0)); 
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
    lexing_data{lexing_data, lexing_data,
                lexing_data + input_length,
                lexing_data - 1, lexing_data,
                1, 1} {
  buildDFA(regex);
}

Token Lexer::nextToken() {

  while (lexing_data.itr != lexing_data.end) {
    unsigned int new_state = simulateChar(*lexing_data.itr);
    if (isFinishState(new_state)) {
      return Token(lexing_data, TokenIdentifier::END_OF_FILE, 1);
    }
    ++lexing_data.itr;
  }
  return Token(lexing_data, TokenIdentifier::END_OF_FILE, 0);
}

unsigned int Lexer::simulateChar(const char letter) {
  return 0;
}

bool Lexer::isFinishState(unsigned int state) {
  return 0;
}

void Lexer::reset() {
  lexing_data.itr = lexing_data.begin;

  lexing_data.last_line_begin = lexing_data.begin - 1;
  lexing_data.current_token_begin = lexing_data.begin;

  lexing_data.line_count = 1;
  lexing_data.current_token_column = 1;
}

// TODO: How to deal with |, e.g. (abc)* | bca | cb+
unsigned int Lexer::estimateNumStates(const char* regex) {
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
  //TODO: Should 0 be a default garbage state, and start = 1??
  memset(dfa.states, 0, dfa.num_states * sizeof(*dfa.states) * dfa_width);
  // dfa.states operates as follows
  // dfa.states[STATE_NUMBER * dfa_width + INPUT_CHARACTER] = NEW_STATE_NUMBER

  unsigned int current_state = -1;
  for (const char* itr = regex; *itr != '\0'; ++itr) {
    ++current_state;
    assert(current_state != dfa.num_states);

    switch (*itr) {
      // deal with special metacharacters 
      case '[': {
        // only need one state until we find a matchin ']' 
        // move the iterator to that position and continue
        const char* look_ahead = ++itr;
        const char* first_char = itr;
        while (*look_ahead != ']') {
          if (*look_ahead == '\0') {
            std::cerr << "Bad regular expression: no matching ']' for '['" << std::endl;
          }
          ++look_ahead;
        }
        ++look_ahead;
        switch (*look_ahead) {
          // iterate between [...] and write to transition tables
          case '*': {
            for (itr = first_char; itr != look_ahead - 1; ++itr) {
              dfa.states[current_state * dfa_width + *itr] = current_state;  
            } 
            break;
          }  
          case '+': {
            for (itr = first_char; itr != look_ahead - 1; ++itr) {
              unsigned int next_state = current_state + 1;
              dfa.states[current_state * dfa_width + *itr] = next_state;  
              dfa.states[next_state * dfa_width + *itr] = next_state;
            }
            break;
          }
          default: {
            for (itr = first_char; itr != look_ahead - 1; ++itr) {
              unsigned int next_state = current_state + 1;
              dfa.states[current_state * dfa_width + *itr] = next_state;  
            }
            break;
          }
        }
        ++itr;
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
        unsigned int next_state = current_state + 1;
        dfa.states[current_state * dfa_width + *itr] = next_state;  
         
        // normal character
        break;
      }
    }
  }
}

