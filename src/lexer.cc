
#include "lexer.h"

#include <regex>
#include <cstring>
#include <vector>
#include <iostream>
#include <cassert>

#include "file_mapped_io.h"

struct LexingIterator {
  const char* begin;
  const char* end;
  const char* itr;

  const char* last_line_begin;
  int line_count;
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

internal_ void
eatMeaninglessChars(LexingIterator& lexer) {
  for (;;) {
    switch (*lexer.itr) {
      case '\t': break;
      case '\r': break;
      case ' ': break;
      case '\n': {
        last_line_begin = lexer.itr;
        ++lexer.itr;
        ++lexer.line_count;
        break;
      }
      default: return;
    }
  }
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

  LexingIterator lexer = { file_begin, file_end, file_begin, 0 };
  std::vector<Token> out_tokens;
  Token token;

  for (; lexer.itr != lexer.end; ++lexer.itr) {
    
    eatMeaninglessChars(lexer);
    
    switch (lexer.itr) {
      case OPEN_PARANTHESIS: {
      }
    }
    
  }
   
  return std::vector<Token>();
}
