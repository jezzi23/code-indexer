
// Print test for the lexer.

#include <cstdio>

#include "lexer.h"
#include "index_database.h"
#include "file_mapped_io.h"


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

  Lexer lexer(file_begin, file_end);

  enum {
    OCCUPIED = 0,
    WHITE_SPACE_FOOD,
    INTEGER_NUM,
    FLOAT_NUM,
    NAME,
    COMMENT,
    DELIMITER,
    END_OF_FILE = -52
  };

  // Example rules
  lexer.addRule("/\\*(\\*[^/]|[^*])*\\*/", COMMENT);
  lexer.addRule("[0-9]*\\.[0-9]+", FLOAT_NUM);
  lexer.addRule("[0-9]+", INTEGER_NUM);
  lexer.addRule("[a-zA-Z_]+", NAME);
  lexer.addRule("{|}|\\(|\\)|,|;|\\[|\\]|<|>|\\.", DELIMITER);
  lexer.addRule("\n|\r|\t| ", WHITE_SPACE_FOOD);

  lexer.build();

  Token token;
  int count = 0;
  for (;;) {
    token = lexer.nextToken();
    if (token.id == END_OF_FILE) {
      printf("%s\n", "EOF reached");
      break;
    }
    count++;

    printf("Token.\nIndex: %d\nLength: %d\nTokenId: %d\nline_count: %d\nColumn_count: %d\nContents:\n%.*s\n\n",
      (int)token.index,
      (int)token.length,
      (int)token.id,
      (int)token.line_count,
      (int)token.column_count,
      (int)token.length,
      file_begin + token.index);
  }
  printf("%d occurrences", count);

  return EXIT_SUCCESS;
}

