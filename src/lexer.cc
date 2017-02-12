
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
  if (!hasFileExtension(filename, "cpp")) {
    std::cout << "Loaded unsupported file format: " << filename << std::endl; 
  }

  FileMapper filemap(filename);
  u32 file_size = static_cast<u32>(filemap.getFileSize());
  
  const char* file_begin = static_cast<const char*>(filemap.map(0, file_size));
  const char* file_end   = file_begin + file_size;

  LexingIterator lexer = { file_begin, file_end, file_begin, file_begin - 1, 1 };
  std::vector<Token> out_tokens;

  for (; lexer.itr != lexer.end; ++lexer.itr) {

    if (out_tokens.size() == 3004) {
      int breakm = 5;
    }

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

