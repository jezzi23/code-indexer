
#ifndef REGEX_QUANTIFICATION_H_
#define REGEX_QUANTIFICATION_H_

// Expression group quantification:
//
// "*" : zero or more occurrences
// "+" : one or more occurrences 
// "?" : zero or one occurences 
// "{n}" : exactly n occurrences
// "{n,}" : n or more occurrences
// "{n,m}": between n and m occurences (inclusive)
// "{,m}" : between zero and m occurences
struct ExpressionGroupQuantification {
  ExpressionGroupQuantification(int min = 1, int max = 1);
  ExpressionGroupQuantification(const char* quantifier_begin);
  
  int quantifyOnString(const char* quantification_begin);

  int min_occurrences;
  int max_occurrences;

  const int INFINITE_OCCURRENCES = -1;
};

#endif // REGEX_QUANTIFICATION_H_
