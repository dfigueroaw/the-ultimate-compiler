#include "../include/scanner.h"

#include <cctype>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

Scanner::Scanner(const char *s) : input(s), first(0), current(0) {}

namespace {
std::unique_ptr<Token> makeToken(Token::Type type, const std::string &input,
                                 std::size_t first, std::size_t length) {
  return std::make_unique<Token>(type, input.substr(first, length));
}

std::optional<Token::Type> keywordType(const std::string &lexeme) {
  static const std::unordered_map<std::string, Token::Type> keywords{
      {"if", Token::IF},
      {"else", Token::ELSE},
      {"while", Token::WHILE},
      {"for", Token::FOR},
      {"do", Token::DO},
      {"break", Token::BREAK},
      {"continue", Token::CONTINUE},
      {"switch", Token::SWITCH},
      {"case", Token::CASE},
      {"default", Token::DEFAULT},
      {"struct", Token::STRUCT},
      {"sizeof", Token::SIZEOF},
      {"int", Token::INT},
      {"void", Token::VOID},
      {"char", Token::CHAR},
      {"short", Token::SHORT},
      {"long", Token::LONG},
      {"signed", Token::SIGNED},
      {"unsigned", Token::UNSIGNED},
      {"return", Token::RETURN},
  };

  if (const auto found = keywords.find(lexeme); found != keywords.end())
    return found->second;
  return std::nullopt;
}

std::optional<Token::Type> twoCharType(char c, char next) {
  switch (c) {
  case '+':
    if (next == '=')
      return Token::PLUS_ASSIGN;
    return next == '+' ? std::optional{Token::PLUSPLUS} : std::nullopt;
  case '*':
    return next == '=' ? std::optional{Token::MUL_ASSIGN} : std::nullopt;
  case '/':
    return next == '=' ? std::optional{Token::DIV_ASSIGN} : std::nullopt;
  case '%':
    return next == '=' ? std::optional{Token::MOD_ASSIGN} : std::nullopt;
  case '=':
    return next == '=' ? std::optional{Token::EQ} : std::nullopt;
  case '<':
    return next == '=' ? std::optional{Token::LEQ} : std::nullopt;
  case '>':
    return next == '=' ? std::optional{Token::GEQ} : std::nullopt;
  case '!':
    return next == '=' ? std::optional{Token::NE} : std::nullopt;
  case '-':
    if (next == '=')
      return Token::MINUS_ASSIGN;
    if (next == '>')
      return Token::ARROW;
    if (next == '-')
      return Token::MINUSMINUS;
    return std::nullopt;
  case '&':
    return next == '&' ? std::optional{Token::AND} : std::nullopt;
  case '|':
    return next == '|' ? std::optional{Token::OR} : std::nullopt;
  default:
    return std::nullopt;
  }
}

std::optional<Token::Type> threeCharType(char c, char next, char third) {
  if (c == '.' && next == '.' && third == '.')
    return Token::ELLIPSIS;
  return std::nullopt;
}

std::optional<Token::Type> oneCharType(char c) {
  switch (c) {
  case '+':
    return Token::PLUS;
  case '-':
    return Token::MINUS;
  case '*':
    return Token::MUL;
  case '/':
    return Token::DIV;
  case '%':
    return Token::MOD;
  case '(':
    return Token::LPAREN;
  case ')':
    return Token::RPAREN;
  case '[':
    return Token::LBRACKET;
  case ']':
    return Token::RBRACKET;
  case '{':
    return Token::LBRACE;
  case '}':
    return Token::RBRACE;
  case ';':
    return Token::SEMICOL;
  case ',':
    return Token::COMA;
  case '.':
    return Token::DOT;
  case ':':
    return Token::COLON;
  case '=':
    return Token::ASSIGN;
  case '<':
    return Token::LE;
  case '>':
    return Token::GT;
  case '!':
    return Token::NOT;
  case '&':
    return Token::AMP;
  default:
    return std::nullopt;
  }
}
} // namespace

std::unique_ptr<Token> Scanner::nextToken() {
  while (current < input.length()) {
    while (current < input.length() &&
           std::isspace(static_cast<unsigned char>(input[current])))
      current++;

    if (current + 1 >= input.length() || input[current] != '/')
      break;

    if (input[current + 1] == '/') {
      current += 2;
      while (current < input.length() && input[current] != '\n')
        current++;
      continue;
    }

    if (input[current + 1] == '*') {
      current += 2;
      bool closed = false;
      while (current + 1 < input.length()) {
        if (input[current] == '*' && input[current + 1] == '/') {
          current += 2;
          closed = true;
          break;
        }
        current++;
      }
      if (!closed) {
        current = input.length();
        return std::make_unique<Token>(Token::ERR, '/');
      }
      continue;
    }

    break;
  }

  if (current >= input.length())
    return std::make_unique<Token>(Token::END);

  char c = input[current];
  first = current;

  if (std::isdigit(static_cast<unsigned char>(c))) {
    current++;
    while (current < input.length() &&
           std::isdigit(static_cast<unsigned char>(input[current])))
      current++;
    return makeToken(Token::NUM, input, first, current - first);
  }

  if (c == '\'') {
    current++;
    if (current >= input.length())
      return std::make_unique<Token>(Token::ERR, c);
    int value = static_cast<unsigned char>(input[current]);
    if (input[current] == '\\') {
      current++;
      if (current >= input.length())
        return std::make_unique<Token>(Token::ERR, c);
      switch (input[current]) {
      case 'n':
        value = '\n';
        break;
      case 't':
        value = '\t';
        break;
      case '0':
        value = '\0';
        break;
      default:
        value = static_cast<unsigned char>(input[current]);
        break;
      }
    }
    current++;
    if (current >= input.length() || input[current] != '\'')
      return std::make_unique<Token>(Token::ERR, c);
    current++;
    return std::make_unique<Token>(Token::NUM, std::to_string(value));
  }

  if (c == '"') {
    current++;
    std::string value;
    while (current < input.length() && input[current] != '"') {
      if (input[current] == '\n')
        return std::make_unique<Token>(Token::ERR, c);
      if (input[current] == '\\') {
        current++;
        if (current >= input.length())
          return std::make_unique<Token>(Token::ERR, c);
        switch (input[current]) {
        case 'n':
          value.push_back('\n');
          break;
        case 't':
          value.push_back('\t');
          break;
        case '0':
          value.push_back('\0');
          break;
        case '"':
          value.push_back('"');
          break;
        case '\\':
          value.push_back('\\');
          break;
        default:
          value.push_back(input[current]);
          break;
        }
      } else {
        value.push_back(input[current]);
      }
      current++;
    }
    if (current >= input.length() || input[current] != '"')
      return std::make_unique<Token>(Token::ERR, c);
    current++;
    return std::make_unique<Token>(Token::STRING, std::move(value));
  }

  if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
    current++;
    while (current < input.length() &&
           (std::isalnum(static_cast<unsigned char>(input[current])) ||
            input[current] == '_'))
      current++;
    std::string lexeme = input.substr(first, current - first);

    return makeToken(keywordType(lexeme).value_or(Token::ID), input, first,
                     current - first);
  }

  if (current + 2 < input.length()) {
    if (const auto type =
            threeCharType(c, input[current + 1], input[current + 2])) {
      const auto text = input.substr(current, 3);
      current += 3;
      return std::make_unique<Token>(*type, text);
    }
  }

  if (current + 1 < input.length()) {
    if (const auto type = twoCharType(c, input[current + 1])) {
      const auto text = input.substr(current, 2);
      current += 2;
      return std::make_unique<Token>(*type, text);
    }
  }

  if (const auto type = oneCharType(c)) {
    current++;
    return std::make_unique<Token>(*type, c);
  }

  current++;
  return std::make_unique<Token>(Token::ERR, c);
}
