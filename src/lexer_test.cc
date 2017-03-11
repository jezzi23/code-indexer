
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
  // Note: C++11 raw string is useful for regex descriptions.

  lexer.addRule(R"(/\*(\*[^/]|[^*])*\*/)", COMMENT);
  lexer.addRule(R"([0-9]*\.[0-9]+)", FLOAT_NUM);
  lexer.addRule(R"([0-9]+)", INTEGER_NUM);
  lexer.addRule(R"([a-zA-Z_]+)", NAME);
  lexer.addRule(R"({|}|\(|\)|,|;|\[|\]|<|>|\.)", DELIMITER);
  //lexer.addRule("\n|\r|\t| ", WHITE_SPACE_FOOD);

  lexer.build();

  Token token;
  int count = 0;

  for (;;) {

    token = lexer.nextToken();
    if (token.id == END_OF_FILE) {
      std::cout << "EOF reached" << std::endl;
      break;
    }

    count++;

    std::cout << "Token found."                       << '\n';
    std::cout << "Index:\t"     << token.index        << '\n';
    std::cout << "Length:\t"    << token.length       << '\n';
    std::cout << "Id:\t"        << token.id           << '\n';
    std::cout << "Line:\t"      << token.line_count   << '\n';
    std::cout << "Column:\t"    << token.column_count << std::endl;
    printf("Contents:\n%.*s\n\n", token.length, file_begin + token.index);
  }
  std::cout << "%d Total occurrences: " << count << std::endl;
  
  filemap.unmap(const_cast<char*>(file_begin), file_size);

  return EXIT_SUCCESS;
}

