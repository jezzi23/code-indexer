
#include "lexer.h"

#include <regex>
#include <cstring>
#include <vector>
#include <iostream>
#include <cassert>

#include "file_mapped_io"

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

  return strcmp(filename, extension); 
}

std::vector<Token>
tokenize(const char* filename) {
  if (!hasFileExtension(file_name, "cpp") {
    std::cout << "Loaded unsupported file format: " << filename << std::endl; 
  }

  FileMapper filemap(filename);
  u64 file_size = filemap.fileSize();
  
  const char* file_begin = filemap.map(0, file_size);
  const char* file_end   = file_begin + file_size;

  for (auto itr = file_begin; itr != file_end; ++itr) {
    auto read_val_test = *itr;
  }
}
