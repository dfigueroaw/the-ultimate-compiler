#ifndef SCANNER_H
#define SCANNER_H

#include "token.h"

#include <memory>
#include <string>

class Scanner {
private:
  std::string input;
  std::size_t first;
  std::size_t current;

public:
  Scanner(const char *in_s);
  std::unique_ptr<Token> nextToken();
};

#endif
