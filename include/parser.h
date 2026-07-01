#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "scanner.h"

#include <memory>
#include <string>

class Parser {
private:
  Scanner *scanner;
  std::unique_ptr<Token> current;
  std::unique_ptr<Token> previous;

  bool match(Token::Type ttype);
  bool check(Token::Type ttype);
  bool advance();
  bool isAtEnd();

  void error(const std::string &expected);

  void expect(Token::Type ttype);
  std::string expectIdentifier(const std::string &expected);
  std::string parseTypeSpecifier();
  bool isTypeSpecifierStart() const;
  Declarator parseDeclarator();
  Declarator parseParameterDeclarator();
  std::unique_ptr<FunctionCallExpression> parseCallAfterName(std::string name);
  std::unique_ptr<Expression>
  parsePostfix(std::unique_ptr<Expression> expression);
  void parseParameterList(FunctionDeclaration &function);
  void addParameter(FunctionDeclaration &function, std::string type,
                    Declarator parameter);
  std::unique_ptr<VariableDeclaration>
  parseVariableDeclarationAfterType(std::string type);
  std::unique_ptr<VariableDeclaration>
  parseVariableDeclarationAfterFirstDeclarator(std::string type,
                                               Declarator first);
  std::unique_ptr<FunctionDeclaration>
  parseFunctionDeclarationAfterSignature(std::string type,
                                         Declarator declarator);
  std::unique_ptr<Statement>
  parseAssignmentFromTarget(std::unique_ptr<Expression> target,
                            Token::Type terminator = Token::SEMICOL);
  std::unique_ptr<Statement>
  parseIDStatement(Token::Type terminator = Token::SEMICOL);
  std::unique_ptr<Statement>
  parseAssignmentAfterName(std::string variable,
                           Token::Type terminator = Token::SEMICOL);
  std::unique_ptr<Statement>
  parsePointerAssignment(Token::Type terminator = Token::SEMICOL);
  void addDeclarationOrFunction(Program &program, std::string type,
                                Declarator declarator);
  ForInitializer parseForInitializer();
  std::unique_ptr<Statement> parseForControlStatement(Token::Type terminator);
  std::unique_ptr<BlockItem> parseBlockItem();

public:
  Parser(Scanner *scanner);

  std::unique_ptr<Program> parseProgram();
  std::unique_ptr<Body> parseBlock();
  std::unique_ptr<VariableDeclaration> parseVariableDeclaration();
  std::unique_ptr<Statement> parseStm();
  std::unique_ptr<Expression> parseCE();
  std::unique_ptr<Expression> parseLogicalOr();
  std::unique_ptr<Expression> parseLogicalAnd();
  std::unique_ptr<Expression> parseEquality();
  std::unique_ptr<Expression> parseRelExp();
  std::unique_ptr<Expression> parseBE();
  std::unique_ptr<Expression> parseE();
  std::unique_ptr<Expression> parseF();
};

#endif
