
// Print test for the lexer.

#include <cstdio>

#include "lexer.h"
#include "index_database.h"
#include "file_mapped_io.h"
#include "cpp_lexer.h"

// args[1]: filename to be lexed
int main(int argc, char* args[]) {

  if (argc != 2) {
    puts("Expected one argument : name of file to be lexed");
    exit(EXIT_FAILURE);
  }

  FileMapper filemap(args[1]);
  u32 file_size = static_cast<u32>(filemap.getFileSize());

  const char* file_begin = static_cast<const char*>(filemap.map(0, file_size));
  const char* file_end = file_begin + file_size;
  
  // Build cpp lexing ruleset
  buildCppLexer();
  // Feed our file data stream
  feedLexer(file_begin, file_end); 

  // Test calls
  lexerTestPrintAllTokens();
  lexerTestPrintAllFunctionsCalledInFunctions();

  // Cleanup
  filemap.unmap(const_cast<char*>(file_begin), file_size);
  
  return EXIT_SUCCESS;
}

