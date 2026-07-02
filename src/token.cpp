#include "../include/token.h"

#include <string>
#include <utility>

Token::Token(Type type) : type(type), text("") {}

Token::Token(Type type, char c) : type(type), text(std::string(1, c)) {}

Token::Token(Type type, std::string text) : type(type), text(std::move(text)) {}

std::string Token::typeName(Type t) {
  switch (t) {
  case PLUS:
    return "'+'";
  case MINUS:
    return "'-'";
  case PLUSPLUS:
    return "'++'";
  case MINUSMINUS:
    return "'--'";
  case MUL:
    return "'*'";
  case DIV:
    return "'/'";
  case MOD:
    return "'%'";
  case LPAREN:
    return "'('";
  case RPAREN:
    return "')'";
  case LBRACKET:
    return "'['";
  case RBRACKET:
    return "']'";
  case LBRACE:
    return "'{'";
  case RBRACE:
    return "'}'";
  case SEMICOL:
    return "';'";
  case COMA:
    return "','";
  case DOT:
    return "'.'";
  case ELLIPSIS:
    return "'...'";
  case ARROW:
    return "'->'";
  case COLON:
    return "':'";
  case LE:
    return "'<'";
  case GT:
    return "'>'";
  case LEQ:
    return "'<='";
  case GEQ:
    return "'>='";
  case EQ:
    return "'=='";
  case NE:
    return "'!='";
  case AND:
    return "'&&'";
  case AMP:
    return "'&'";
  case OR:
    return "'||'";
  case NOT:
    return "'!'";
  case ASSIGN:
    return "'='";
  case PLUS_ASSIGN:
    return "'+='";
  case MINUS_ASSIGN:
    return "'-='";
  case MUL_ASSIGN:
    return "'*='";
  case DIV_ASSIGN:
    return "'/='";
  case MOD_ASSIGN:
    return "'%='";
  case NUM:
    return "número";
  case STRING:
    return "literal de cadena";
  case ID:
    return "identificador";
  case IF:
    return "'if'";
  case ELSE:
    return "'else'";
  case WHILE:
    return "'while'";
  case FOR:
    return "'for'";
  case DO:
    return "'do'";
  case BREAK:
    return "'break'";
  case CONTINUE:
    return "'continue'";
  case SWITCH:
    return "'switch'";
  case CASE:
    return "'case'";
  case DEFAULT:
    return "'default'";
  case SIZEOF:
    return "'sizeof'";
  case STRUCT:
    return "'struct'";
  case INT:
    return "'int'";
  case VOID:
    return "'void'";
  case CHAR:
    return "'char'";
  case SHORT:
    return "'short'";
  case LONG:
    return "'long'";
  case SIGNED:
    return "'signed'";
  case UNSIGNED:
    return "'unsigned'";
  case RETURN:
    return "'return'";
  case ERR:
    return "<error léxico>";
  case END:
    return "fin de entrada";
  }
  return "<desconocido>";
}
