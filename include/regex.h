
#ifndef REGEX_H_
#define REGEX_H_

// Supported metacharacters:
//
//  .      Matches any character
//
//  []     Bracket expression. Matches a single character within the brackets.
//           Example: [ab] matches a or b
//         A range of characters is allowed with - between characters.
//           Example: [A-Z0-9] matches upper case alphabet characters aswell as digits.
//
//  [^]    Inversed bracket expression. Matches all characters except the ones within
//         the brackets.
//           Example: [^ab] matches any character except a and b.
//                    [^a-b] matches all characters except lower case alphabet characters.
//
//  ()     Marks an expression group that may accept an expression group quantifier.
//
//  |      Choice operator. Matches the expression before or after.
//           Example:  abc|fgh matches abc or fgh
//           Example:  ab(c|f)gh matches abcgh or abfgh
//
// Expression group quantifiers:
//
//  *      zero or more occurrences
//  +      one or more occurrences 
//  ?      zero or one occurences 
//  {n}    exactly n occurrences
//  {n,}"  n or more occurrences
//  {n,m}" between n and m occurences (inclusive)
//  {,m}"  between zero and m occurences (inclusive)
struct Regexpr {
  Regexpr() = delete;
  Regexpr(const char* expr);

  const char* expr_begin;
  const char* expr_end;
};

struct ExpressionGroupQuantification {
  ExpressionGroupQuantification(int min = 1, int max = 1);
  ExpressionGroupQuantification(const char* quantifier_begin);
  
  int quantifyOnString(const char* quantification_begin);

  int min_occurrences;
  int max_occurrences;

  const int INFINITE_OCCURRENCES = -1;
};

#endif // REGEX_H_
