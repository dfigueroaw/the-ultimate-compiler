#ifndef TOKEN_H
#define TOKEN_H

#include <string>

class Token {
public:
  enum Type {

    PLUS,
    MINUS,
    PLUSPLUS,
    MINUSMINUS,
    MUL,
    DIV,
    MOD,

    LPAREN,
    RPAREN,
    LBRACKET,
    RBRACKET,
    LBRACE,
    RBRACE,
    SEMICOL,
    COMA,
    DOT,
    ELLIPSIS,
    ARROW,
    COLON,

    LE,
    GT,
    LEQ,
    GEQ,
    EQ,
    NE,

    AND,
    AMP,
    OR,
    NOT,

    ASSIGN,
    PLUS_ASSIGN,
    MINUS_ASSIGN,
    MUL_ASSIGN,
    DIV_ASSIGN,
    MOD_ASSIGN,

    NUM,
    STRING,

    ID,

    IF,
    ELSE,
    WHILE,
    FOR,
    DO,
    BREAK,
    CONTINUE,
    SWITCH,
    CASE,
    DEFAULT,
    SIZEOF,

    STRUCT,
    INT,
    VOID,
    CHAR,
    SHORT,
    LONG,
    SIGNED,
    UNSIGNED,

    RETURN,

    ERR,
    END
  };

  Type type;
  std::string text;

  Token(Type type);
  Token(Type type, char c);
  Token(Type type, std::string text);

  static std::string typeName(Type t);
};

#endif
