
#include "nfa.h"

#include <cassert>
#include <cerrno>

#include "types.h"

ExpressionGroupQuantification::ExpressionGroupQuantification(int min, int max) :
    min_occurrences(min), max_occurrences(max) {
}

ExpressionGroupQuantification::ExpressionGroupQuantification(const char* quantifier_begin) {
  int is_quantified = quantifyOnString(quantifier_begin);
  if (!is_quantified) {
    // No quantification, e.g. (abcd) as opposed to (abcd)*
    // -> appear exactly once
    min_occurrences = max_occurrences = 1;
  }
}
// Returns the length of quantification description
// If there is no such description, 0 is returned
int
ExpressionGroupQuantification::quantifyOnString(const char* quantifier_begin) {

  switch (*quantifier_begin) {
    case '*': {
      min_occurrences = 0;
      max_occurrences = INFINITE_OCCURRENCES;
      return 1;
    }
    case '+': {
      min_occurrences = 1;
      max_occurrences = INFINITE_OCCURRENCES;
      return 1;
    }
    case '?': {
      min_occurrences = 0;
      max_occurrences = 1;
      return 1;
    }
    case '{': {
      const size_t num_str_max_length = 10;
      char atoi_buffer[num_str_max_length + 1] = { 0 };

      const char* quantifier_itr = quantifier_begin + 1;
      const char* num_begin = quantifier_itr;
      u8 num_count = 0;

      // walk through {....} and write min -or max_occurrences
      for (;;) {
        if (*quantifier_itr == '}' || *quantifier_itr == ',') {
          ++num_count; // we have attempted to read a number from string range
          const size_t parsed_num_length = quantifier_itr - num_begin;
          if (parsed_num_length > num_str_max_length) {
            return 0; // number value in string is too long
          }
          memcpy(atoi_buffer, num_begin, parsed_num_length);
          errno = 0;
          int num_val = strtol(atoi_buffer, nullptr, 10);
          if (errno == ERANGE && parsed_num_length != 0) {
            // strtol failed because atoi_buffer is not a valid number as a string
            // "" string of length 0 is allowed for default values
            return 0;
          }
          memset(atoi_buffer, 0, parsed_num_length); // reset buffer

          switch (*quantifier_itr) {
            case '}': {
              assert(num_count >= 1 && num_count <= 2);
              if (num_count <= 1) {
                // we are looking at {n} exact match number
                if (parsed_num_length == 0) {
                  // empty {} not allowed
                  return 0;
                } else {
                  max_occurrences = min_occurrences = num_val;
                }
              } else {
                // we are looking at {n,m} where n or m can be empty ""
                // and m is the value num_val
                max_occurrences = num_val;
              }
              // returns how far we looked for a quantification
              // e.g. {502,612} would return 9
              return (quantifier_itr - quantifier_begin) + 1;
            }
            case ',': {
              assert(num_count == 1);
              if (parsed_num_length == 0) {
                min_occurrences = 0;
              } else {
                min_occurrences = num_val;
              }
              break;
            }
          }
        } else if (*quantifier_itr == '\0') {
          return 0;
        }
        ++quantifier_itr;
      }
    }
  }
  // if we make it to here, no known quantifier was found
  return 0;
}
