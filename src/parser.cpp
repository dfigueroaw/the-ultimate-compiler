#include "../include/parser.h"

#include <algorithm>
#include <array>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace {
bool isCompoundAssignment(Token::Type type) {
  return type == Token::PLUS_ASSIGN || type == Token::MINUS_ASSIGN ||
         type == Token::MUL_ASSIGN || type == Token::DIV_ASSIGN ||
         type == Token::MOD_ASSIGN;
}

BinaryOp compoundAssignmentOp(Token::Type type) {
  if (type == Token::PLUS_ASSIGN)
    return PLUS_OP;
  if (type == Token::MINUS_ASSIGN)
    return MINUS_OP;
  if (type == Token::MUL_ASSIGN)
    return MUL_OP;
  if (type == Token::DIV_ASSIGN)
    return DIV_OP;
  if (type == Token::MOD_ASSIGN)
    return MOD_OP;
  throw std::runtime_error("Operador de asignación compuesta inválido");
}
} // namespace

Parser::Parser(Scanner *sc) : scanner(sc), current(scanner->nextToken()) {
  if (current->type == Token::ERR)
    throw std::runtime_error("Error léxico: carácter no reconocido '" +
                             current->text + "'");
}

bool Parser::isAtEnd() { return current->type == Token::END; }

bool Parser::check(Token::Type ttype) {
  return !isAtEnd() && current->type == ttype;
}

bool Parser::advance() {
  if (isAtEnd())
    return false;

  previous = std::move(current);
  current = scanner->nextToken();
  if (current->type == Token::ERR)
    throw std::runtime_error("Error léxico: carácter no reconocido '" +
                             current->text + "'");
  return true;
}

bool Parser::match(Token::Type ttype) {
  if (!check(ttype))
    return false;
  advance();
  return true;
}

void Parser::error(const std::string &expected) {
  std::string found;
  if (isAtEnd()) {
    found = "fin de entrada";
  } else {
    found = Token::typeName(current->type);
    if (!current->text.empty())
      found += " '" + current->text + "'";
  }
  throw std::runtime_error("Error sintáctico: se esperaba " + expected +
                           ", pero se encontró " + found);
}

void Parser::expect(Token::Type ttype) {
  if (!match(ttype))
    error(Token::typeName(ttype));
}

std::string Parser::expectIdentifier(const std::string &expected) {
  if (!match(Token::ID))
    error(expected);
  return previous->text;
}

std::unique_ptr<Program> Parser::parseProgram() {
  auto program = std::make_unique<Program>();

  while (!isAtEnd()) {
    if (match(Token::STRUCT)) {
      const auto name = expectIdentifier("nombre de estructura");
      if (check(Token::LBRACE)) {
        auto structure = std::make_unique<StructDeclaration>();
        structure->name = "struct " + name;

        expect(Token::LBRACE);
        while (!check(Token::RBRACE)) {
          structure->fields.push_back(parseVariableDeclaration());
          expect(Token::SEMICOL);
        }
        expect(Token::RBRACE);
        expect(Token::SEMICOL);
        program->structDeclarations.push_back(std::move(structure));
        continue;
      }

      addDeclarationOrFunction(*program, "struct " + name,
                              parseDeclarator());
      continue;
    }

    const auto type = parseTypeSpecifier();
    addDeclarationOrFunction(*program, std::move(type),
                            parseDeclarator());
  }

  return program;
}

std::unique_ptr<VariableDeclaration> Parser::parseVariableDeclaration() {
  const auto type = parseTypeSpecifier();
  return parseVariableDeclarationAfterType(type);
}

std::string Parser::parseTypeSpecifier() {
  if (match(Token::STRUCT))
    return "struct " + expectIdentifier("nombre de estructura");
  if (match(Token::UNSIGNED)) {
    if (match(Token::CHAR))
      return "unsigned char";
    if (match(Token::SHORT)) {
      match(Token::INT);
      return "unsigned short";
    }
    if (match(Token::LONG)) {
      if (match(Token::LONG)) {
        match(Token::INT);
        return "unsigned long long";
      }
      match(Token::INT);
      return "unsigned long";
    }
    match(Token::INT);
    return "unsigned int";
  }
  if (match(Token::INT))
    return "int";
  if (match(Token::VOID))
    return "void";
  if (match(Token::CHAR))
    return "char";
  if (match(Token::SHORT)) {
    match(Token::INT);
    return "short";
  }
  if (match(Token::LONG)) {
    if (match(Token::LONG)) {
      match(Token::INT);
      return "long long";
    }
    match(Token::INT);
    return "long";
  }
  return expectIdentifier("nombre de tipo");
}

bool Parser::isTypeSpecifierStart() const {
  return current->type == Token::STRUCT || current->type == Token::INT ||
         current->type == Token::VOID || current->type == Token::CHAR ||
         current->type == Token::SHORT || current->type == Token::LONG ||
         current->type == Token::UNSIGNED;
}

Declarator Parser::parseDeclarator() {
  Declarator declarator;
  while (match(Token::MUL))
    declarator.pointerDepth++;
  declarator.name = expectIdentifier("nombre de declarador");
  while (match(Token::LBRACKET)) {
    declarator.dimensions.push_back(parseCE());
    expect(Token::RBRACKET);
  }
  if (match(Token::ASSIGN))
    declarator.initializer = parseCE();
  return declarator;
}

Declarator Parser::parseParameterDeclarator() {
  Declarator declarator;
  while (match(Token::MUL))
    declarator.pointerDepth++;
  if (match(Token::ID)) {
    declarator.name = previous->text;
    while (match(Token::LBRACKET)) {
      declarator.dimensions.push_back(parseCE());
      expect(Token::RBRACKET);
    }
  }
  if (match(Token::ASSIGN))
    declarator.initializer = parseCE();
  return declarator;
}

std::unique_ptr<FunctionCallExpression>
Parser::parseCallAfterName(std::string name) {
  expect(Token::LPAREN);
  auto call = std::make_unique<FunctionCallExpression>();
  call->name = std::move(name);
  if (!check(Token::RPAREN)) {
    call->arguments.push_back(parseCE());
    while (match(Token::COMA))
      call->arguments.push_back(parseCE());
  }
  expect(Token::RPAREN);
  return call;
}

std::unique_ptr<VariableDeclaration>
Parser::parseVariableDeclarationAfterType(std::string type) {
  return parseVariableDeclarationAfterFirstDeclarator(std::move(type),
                                                      parseDeclarator());
}

std::unique_ptr<VariableDeclaration>
Parser::parseVariableDeclarationAfterFirstDeclarator(std::string type,
                                                     Declarator first) {
  auto declaration = std::make_unique<VariableDeclaration>();
  declaration->baseType = std::move(type);
  declaration->declarators.push_back(std::move(first));

  while (match(Token::COMA))
    declaration->declarators.push_back(parseDeclarator());

  return declaration;
}

void Parser::addDeclarationOrFunction(Program &program, std::string type,
                                       Declarator declarator) {
  if (check(Token::LPAREN)) {
    program.functionDeclarations.push_back(
        parseFunctionDeclarationAfterSignature(std::move(type),
                                               std::move(declarator)));
  } else {
    program.globalDeclarations.push_back(
        parseVariableDeclarationAfterFirstDeclarator(std::move(type),
                                                     std::move(declarator)));
    expect(Token::SEMICOL);
  }
}

void Parser::addParameter(FunctionDeclaration &function, std::string type,
                          Declarator parameter) {
  if (!parameter.dimensions.empty())
    throw std::runtime_error(
        "Error sintáctico: los parámetros con sintaxis de arreglo no están "
        "soportados; use punteros explícitos");
  if (parameter.initializer)
    throw std::runtime_error(
        "Error sintáctico: los parámetros no pueden tener inicializador");

  function.parameterBaseTypes.push_back(std::move(type));
  function.parameterPointerDepths.push_back(parameter.pointerDepth);
  function.parameterNames.push_back(std::move(parameter.name));
}

void Parser::parseParameterList(FunctionDeclaration &function) {
  if (check(Token::RPAREN))
    return;

  if (match(Token::VOID) && check(Token::RPAREN))
    return;

  while (true) {
    if (match(Token::ELLIPSIS)) {
      if (function.parameterBaseTypes.empty())
        error("parámetro antes de '...'");
      function.variadic = true;
      return;
    }

    const auto paramType = previous && previous->type == Token::VOID
                               ? "void"
                               : parseTypeSpecifier();
    addParameter(function, paramType, parseParameterDeclarator());

    if (check(Token::RPAREN))
      return;
    if (!match(Token::COMA))
      error("',' o ')' en la lista de parámetros de '" + function.name + "'");
  }
}

std::unique_ptr<FunctionDeclaration>
Parser::parseFunctionDeclarationAfterSignature(std::string type,
                                               Declarator declarator) {
  auto function = std::make_unique<FunctionDeclaration>();
  if (!declarator.dimensions.empty())
    throw std::runtime_error(
        "Error sintáctico: las funciones no pueden retornar arreglos");
  function->returnBaseType = std::move(type);
  function->returnPointerDepth = declarator.pointerDepth;
  function->name = std::move(declarator.name);
  expect(Token::LPAREN);
  parseParameterList(*function);
  expect(Token::RPAREN);

  if (match(Token::SEMICOL))
    return function;

  function->body = parseBlock();
  return function;
}

std::unique_ptr<BlockItem> Parser::parseBlockItem() {
  if (isTypeSpecifierStart()) {
    auto declaration = parseVariableDeclaration();
    expect(Token::SEMICOL);
    return std::make_unique<DeclarationItem>(std::move(declaration));
  }

  if (check(Token::ID)) {
    auto first = expectIdentifier("inicio de declaración o sentencia");
    if (check(Token::ID) || check(Token::MUL)) {
      auto declaration = parseVariableDeclarationAfterType(std::move(first));
      expect(Token::SEMICOL);
      return std::make_unique<DeclarationItem>(std::move(declaration));
    }
    return std::make_unique<StatementItem>(parseIDStatement());
  }

  return std::make_unique<StatementItem>(parseStm());
}

std::unique_ptr<Body> Parser::parseBlock() {
  auto body = std::make_unique<Body>();
  expect(Token::LBRACE);

  while (!check(Token::RBRACE)) {
    if (isAtEnd())
      error("'}' para cerrar bloque");

    body->items.push_back(parseBlockItem());
  }

  expect(Token::RBRACE);
  return body;
}

std::unique_ptr<Expression>
Parser::parsePostfix(std::unique_ptr<Expression> expression) {
  while (true) {
    if (match(Token::LBRACKET)) {
      std::vector<std::unique_ptr<Expression>> indices;
      indices.push_back(parseCE());
      expect(Token::RBRACKET);
      expression = std::make_unique<SubscriptExpression>(std::move(expression),
                                                         std::move(indices));
      continue;
    }
    if (match(Token::DOT) || match(Token::ARROW)) {
      const bool throughPointer = previous->type == Token::ARROW;
      if (!match(Token::ID))
        error("nombre de campo");
      expression = std::make_unique<FieldExpression>(
          std::move(expression), previous->text, throughPointer);
      continue;
    }
    if (match(Token::PLUSPLUS) || match(Token::MINUSMINUS)) {
      const IncDecOp op = previous->type == Token::PLUSPLUS
                              ? IncDecOp::Increment
                              : IncDecOp::Decrement;
      expression =
          std::make_unique<IncDecExpression>(std::move(expression), op, true);
      continue;
    }
    break;
  }
  return expression;
}

std::unique_ptr<Statement>
Parser::parseAssignmentFromTarget(std::unique_ptr<Expression> target,
                                   Token::Type terminator) {
  if (match(Token::ASSIGN)) {
    auto statement = std::make_unique<AssignmentStatement>(
        std::move(target), parseCE());
    expect(terminator);
    return statement;
  }
  if (!isCompoundAssignment(current->type))
    error("Operacion de asignacion no aceptada. Se espero =");
  const Token::Type op = current->type;
  advance();
  auto statement = std::make_unique<CompoundAssignmentStatement>(
      std::move(target), parseCE(), compoundAssignmentOp(op));
  expect(terminator);
  return statement;
}

std::unique_ptr<Statement>
Parser::parseIDStatement(Token::Type terminator) {
  std::string name = previous->text;

  if (check(Token::LPAREN)) {
    auto call = parseCallAfterName(std::move(name));
    expect(terminator);
    return std::make_unique<ExpressionStatement>(std::move(call));
  }

  std::unique_ptr<Expression> expression =
      std::make_unique<IdentifierExpression>(name);
  expression = parsePostfix(std::move(expression));

  if (auto *incDec = dynamic_cast<IncDecExpression *>(expression.get())) {
    if (incDec->postfix) {
      expect(terminator);
      return std::make_unique<ExpressionStatement>(std::move(expression));
    }
  }

  if (!dynamic_cast<IdentifierExpression *>(expression.get()))
    return parseAssignmentFromTarget(std::move(expression), terminator);

  return parseAssignmentAfterName(name, terminator);
}

std::unique_ptr<Statement>
Parser::parseAssignmentAfterName(std::string variable, Token::Type terminator) {
  std::unique_ptr<Expression> target =
      std::make_unique<IdentifierExpression>(std::move(variable));
  target = parsePostfix(std::move(target));
  return parseAssignmentFromTarget(std::move(target), terminator);
}

std::unique_ptr<Statement>
Parser::parsePointerAssignment(Token::Type terminator) {
  expect(Token::MUL);
  auto target = std::make_unique<DereferenceExpression>(parseF());
  return parseAssignmentFromTarget(std::move(target), terminator);
}

std::unique_ptr<Statement>
Parser::parseForControlStatement(Token::Type terminator) {
  if (check(terminator)) {
    expect(terminator);
    return nullptr;
  }

  if (match(Token::ID))
    return parseIDStatement(terminator);

  if (match(Token::PLUSPLUS) || match(Token::MINUSMINUS)) {
    const IncDecOp op = previous->type == Token::PLUSPLUS ? IncDecOp::Increment
                                                          : IncDecOp::Decrement;
    auto expression = std::make_unique<IncDecExpression>(parseF(), op, false);
    expect(terminator);
    return std::make_unique<ExpressionStatement>(std::move(expression));
  }

  if (check(Token::MUL))
    return parsePointerAssignment(terminator);

  error("asignación o llamada en encabezado de for");
  return nullptr;
}

ForInitializer Parser::parseForInitializer() {
  if (check(Token::SEMICOL)) {
    expect(Token::SEMICOL);
    return std::monostate{};
  }

  if (isTypeSpecifierStart()) {
    auto declaration = parseVariableDeclaration();
    expect(Token::SEMICOL);
    return declaration;
  }

  return parseForControlStatement(Token::SEMICOL);
}

std::unique_ptr<Statement> Parser::parseStm() {
  if (match(Token::ID))
    return parseIDStatement();

  if (match(Token::PLUSPLUS) || match(Token::MINUSMINUS)) {
    const IncDecOp op = previous->type == Token::PLUSPLUS ? IncDecOp::Increment
                                                          : IncDecOp::Decrement;
    auto expression = std::make_unique<IncDecExpression>(parseF(), op, false);
    expect(Token::SEMICOL);
    return std::make_unique<ExpressionStatement>(std::move(expression));
  }

  if (check(Token::MUL))
    return parsePointerAssignment();

  if (match(Token::RETURN)) {
    auto statement = std::make_unique<ReturnStatement>();
    if (!check(Token::SEMICOL))
      statement->expression = parseCE();
    expect(Token::SEMICOL);
    return statement;
  }

  if (match(Token::IF)) {
    expect(Token::LPAREN);
    auto condition = parseCE();
    expect(Token::RPAREN);

    auto thenBody = parseBlock();
    std::unique_ptr<Body> elseBody;

    if (match(Token::ELSE))
      elseBody = parseBlock();

    return std::make_unique<IfStatement>(
        std::move(condition), std::move(thenBody), std::move(elseBody));
  }

  if (match(Token::FOR)) {
    expect(Token::LPAREN);
    auto initializer = parseForInitializer();

    std::unique_ptr<Expression> condition;
    if (check(Token::SEMICOL)) {
      expect(Token::SEMICOL);
    } else {
      condition = parseCE();
      expect(Token::SEMICOL);
    }

    auto update = parseForControlStatement(Token::RPAREN);
    auto body = parseBlock();

    return std::make_unique<ForStatement>(std::move(initializer),
                                          std::move(condition),
                                          std::move(update), std::move(body));
  }

  if (match(Token::WHILE)) {
    expect(Token::LPAREN);
    auto condition = parseCE();
    expect(Token::RPAREN);

    auto body = parseBlock();

    return std::make_unique<WhileStatement>(std::move(condition),
                                            std::move(body));
  }

  if (match(Token::DO)) {
    auto body = parseBlock();

    expect(Token::WHILE);
    expect(Token::LPAREN);
    auto condition = parseCE();
    expect(Token::RPAREN);
    expect(Token::SEMICOL);

    return std::make_unique<DoWhileStatement>(std::move(body),
                                              std::move(condition));
  }

  if (match(Token::BREAK)) {
    expect(Token::SEMICOL);
    return std::make_unique<BreakStatement>();
  }

  if (match(Token::CONTINUE)) {
    expect(Token::SEMICOL);
    return std::make_unique<ContinueStatement>();
  }

  if (match(Token::SWITCH)) {
    expect(Token::LPAREN);
    auto statement = std::make_unique<SwitchStatement>(parseCE());
    expect(Token::RPAREN);
    expect(Token::LBRACE);

    while (match(Token::CASE)) {
      auto caseStatement = std::make_unique<CaseStatement>(parseCE());
      expect(Token::COLON);
      auto isCaseEnd = [&]() {
        return check(Token::CASE) || check(Token::DEFAULT) ||
               check(Token::RBRACE);
      };

      while (!isCaseEnd())
        caseStatement->body.push_back(parseStm());
      statement->cases.push_back(std::move(caseStatement));
    }

    if (match(Token::DEFAULT)) {
      expect(Token::COLON);
      while (!check(Token::RBRACE))
        statement->defaultBody.push_back(parseStm());
    }

    expect(Token::RBRACE);
    return statement;
  }

  error("inicio de sentencia: identificador, 'return', 'if', 'for', 'while', "
        "'do', 'break', 'continue' o 'switch'");
  return nullptr;
}

std::unique_ptr<Expression> Parser::parseCE() { return parseLogicalOr(); }

std::unique_ptr<Expression> Parser::parseLogicalOr() {
  auto left = parseLogicalAnd();
  while (match(Token::OR))
    left = std::make_unique<BinaryExpression>(std::move(left),
                                              parseLogicalAnd(), OR_OP);
  return left;
}

std::unique_ptr<Expression> Parser::parseLogicalAnd() {
  auto left = parseEquality();
  while (match(Token::AND))
    left = std::make_unique<BinaryExpression>(std::move(left), parseEquality(),
                                              AND_OP);
  return left;
}

std::unique_ptr<Expression> Parser::parseEquality() {
  auto left = parseRelExp();
  while (true) {
    static constexpr std::array operators{
        std::pair{Token::EQ, EQ_OP},
        std::pair{Token::NE, NE_OP},
    };

    const auto it =
        std::find_if(operators.begin(), operators.end(),
                     [&](const auto &entry) { return check(entry.first); });
    if (it == operators.end())
      break;

    advance();
    left = std::make_unique<BinaryExpression>(std::move(left), parseRelExp(),
                                              it->second);
  }
  return left;
}

std::unique_ptr<Expression> Parser::parseRelExp() {
  auto left = parseBE();
  while (true) {
    static constexpr std::array operators{
        std::pair{Token::LE, LE_OP},
        std::pair{Token::GT, GT_OP},
        std::pair{Token::LEQ, LEQ_OP},
        std::pair{Token::GEQ, GEQ_OP},
    };

    const auto it =
        std::find_if(operators.begin(), operators.end(),
                     [&](const auto &entry) { return check(entry.first); });
    if (it == operators.end())
      break;

    advance();
    left = std::make_unique<BinaryExpression>(std::move(left), parseBE(),
                                              it->second);
  }
  return left;
}

std::unique_ptr<Expression> Parser::parseBE() {
  auto left = parseE();
  while (match(Token::PLUS) || match(Token::MINUS)) {
    BinaryOp op = (previous->type == Token::PLUS) ? PLUS_OP : MINUS_OP;
    left = std::make_unique<BinaryExpression>(std::move(left), parseE(), op);
  }
  return left;
}

std::unique_ptr<Expression> Parser::parseE() {
  auto left = parseF();
  while (match(Token::MUL) || match(Token::DIV) || match(Token::MOD)) {
    BinaryOp op = MUL_OP;
    if (previous->type == Token::DIV)
      op = DIV_OP;
    else if (previous->type == Token::MOD)
      op = MOD_OP;
    left = std::make_unique<BinaryExpression>(std::move(left), parseF(), op);
  }
  return left;
}

std::unique_ptr<Expression> Parser::parseF() {
  if (match(Token::PLUSPLUS) || match(Token::MINUSMINUS)) {
    const IncDecOp op = previous->type == Token::PLUSPLUS ? IncDecOp::Increment
                                                          : IncDecOp::Decrement;
    return std::make_unique<IncDecExpression>(parseF(), op, false);
  }

  if (match(Token::NOT))
    return std::make_unique<UnaryExpression>(parseF(), NOT_OP);

  if (match(Token::MINUS))
    return std::make_unique<UnaryExpression>(parseF(), NEG_OP);

  if (match(Token::MUL))
    return std::make_unique<DereferenceExpression>(parseF());

  if (match(Token::AMP))
    return std::make_unique<AddressExpression>(parseF());

  if (match(Token::NUM))
    return std::make_unique<NumberExpression>(std::stoll(previous->text));

  if (match(Token::STRING))
    return std::make_unique<StringExpression>(previous->text);

  if (match(Token::LPAREN)) {
    auto expression = parseCE();
    expect(Token::RPAREN);
    return expression;
  }

  if (match(Token::SIZEOF)) {
    expect(Token::LPAREN);
    if (isTypeSpecifierStart()) {
      auto type = parseTypeSpecifier();
      std::size_t pointerDepth = 0;
      while (match(Token::MUL))
        pointerDepth++;
      expect(Token::RPAREN);
      return std::make_unique<SizeofExpression>(std::move(type), pointerDepth);
    }
    auto expression = parseCE();
    expect(Token::RPAREN);
    return std::make_unique<SizeofExpression>(std::move(expression));
  }

  if (match(Token::ID)) {
    std::string name = previous->text;

    if (check(Token::LPAREN)) {
      return parseCallAfterName(std::move(name));
    }

    std::unique_ptr<Expression> expression =
        std::make_unique<IdentifierExpression>(name);
    return parsePostfix(std::move(expression));
  }

  error("expresión: número, identificador o '('");
  return nullptr;
}
