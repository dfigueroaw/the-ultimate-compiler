#ifndef AST_H
#define AST_H

#include <cstddef>
#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <variant>
#include <vector>

class Visitor;
class Expression;
class VariableDeclaration;
class StructDeclaration;
class Body;

struct Declarator {
  std::string name;
  std::size_t pointerDepth = 0;
  std::vector<std::unique_ptr<Expression>> dimensions;
  std::unique_ptr<Expression> initializer;
};

enum BinaryOp {
  PLUS_OP,
  MINUS_OP,
  MUL_OP,
  DIV_OP,
  MOD_OP,
  LE_OP,
  GT_OP,
  LEQ_OP,
  GEQ_OP,
  EQ_OP,
  NE_OP,
  AND_OP,
  OR_OP
};

enum UnaryOp { NOT_OP, NEG_OP };

enum class IncDecOp { Increment, Decrement };

class Expression {
public:
  virtual int accept(Visitor *visitor) = 0;
  virtual ~Expression() = default;
};

class BinaryExpression : public Expression {
public:
  std::unique_ptr<Expression> left;
  std::unique_ptr<Expression> right;
  BinaryOp op;
  BinaryExpression(std::unique_ptr<Expression> l, std::unique_ptr<Expression> r,
                   BinaryOp op);
  int accept(Visitor *visitor) override;
  ~BinaryExpression() override = default;
};

class UnaryExpression : public Expression {
public:
  std::unique_ptr<Expression> operand;
  UnaryOp op;
  UnaryExpression(std::unique_ptr<Expression> operand, UnaryOp op);
  int accept(Visitor *visitor) override;
  ~UnaryExpression() override = default;
};

class IncDecExpression : public Expression {
public:
  std::unique_ptr<Expression> operand;
  IncDecOp op;
  bool postfix;
  IncDecExpression(std::unique_ptr<Expression> operand, IncDecOp op,
                   bool postfix);
  int accept(Visitor *visitor) override;
  ~IncDecExpression() override = default;
};

class NumberExpression : public Expression {
public:
  std::int64_t value;
  explicit NumberExpression(std::int64_t v);
  int accept(Visitor *visitor) override;
  ~NumberExpression() override = default;
};

class StringExpression : public Expression {
public:
  std::string value;
  explicit StringExpression(std::string v);
  int accept(Visitor *visitor) override;
  ~StringExpression() override = default;
};

class IdentifierExpression : public Expression {
public:
  std::string value;
  explicit IdentifierExpression(std::string v);
  int accept(Visitor *visitor) override;
  ~IdentifierExpression() override = default;
};

class SubscriptExpression : public Expression {
public:
  std::unique_ptr<Expression> base;
  std::unique_ptr<Expression> index;
  SubscriptExpression(std::unique_ptr<Expression> base,
                      std::unique_ptr<Expression> index);
  int accept(Visitor *visitor) override;
  ~SubscriptExpression() override = default;
};

class DereferenceExpression : public Expression {
public:
  std::unique_ptr<Expression> operand;
  explicit DereferenceExpression(std::unique_ptr<Expression> operand);
  int accept(Visitor *visitor) override;
  ~DereferenceExpression() override = default;
};

class AddressExpression : public Expression {
public:
  std::unique_ptr<Expression> operand;
  explicit AddressExpression(std::unique_ptr<Expression> operand);
  int accept(Visitor *visitor) override;
  ~AddressExpression() override = default;
};

class SizeofExpression : public Expression {
public:
  std::string baseType;
  std::size_t pointerDepth;
  std::unique_ptr<Expression> expression;
  SizeofExpression(std::string baseType, std::size_t pointerDepth);
  explicit SizeofExpression(std::unique_ptr<Expression> expression);
  int accept(Visitor *visitor) override;
  ~SizeofExpression() override = default;
};

class FunctionCallExpression : public Expression {
public:
  std::string name;
  std::vector<std::unique_ptr<Expression>> arguments;
  FunctionCallExpression() = default;
  int accept(Visitor *visitor) override;
  ~FunctionCallExpression() override = default;
};

class FieldExpression : public Expression {
public:
  std::unique_ptr<Expression> object;
  std::string field;
  bool throughPointer;
  FieldExpression(std::unique_ptr<Expression> object, std::string field,
                  bool throughPointer);
  int accept(Visitor *v) override;
  ~FieldExpression() override = default;
};

class Statement {
public:
  virtual int accept(Visitor *visitor) = 0;
  virtual ~Statement() = default;
};

using ForInitializer =
    std::variant<std::monostate, std::unique_ptr<VariableDeclaration>,
                 std::unique_ptr<Statement>>;

class AssignmentStatement : public Statement {
public:
  std::unique_ptr<Expression> target;
  std::unique_ptr<Expression> expression;
  AssignmentStatement(std::unique_ptr<Expression> target,
                      std::unique_ptr<Expression> expr);
  int accept(Visitor *visitor) override;
  ~AssignmentStatement() override = default;
};

class CompoundAssignmentStatement : public Statement {
public:
  std::unique_ptr<Expression> target;
  std::unique_ptr<Expression> expression;
  BinaryOp op;
  CompoundAssignmentStatement(std::unique_ptr<Expression> target,
                              std::unique_ptr<Expression> expr, BinaryOp op);
  int accept(Visitor *visitor) override;
  ~CompoundAssignmentStatement() override = default;
};

class ExpressionStatement : public Statement {
public:
  std::unique_ptr<Expression> expression;
  explicit ExpressionStatement(std::unique_ptr<Expression> expression);
  int accept(Visitor *visitor) override;
  ~ExpressionStatement() override = default;
};

class ReturnStatement : public Statement {
public:
  std::unique_ptr<Expression> expression;
  ReturnStatement() = default;
  int accept(Visitor *visitor) override;
  ~ReturnStatement() override = default;
};

class IfStatement : public Statement {
public:
  std::unique_ptr<Expression> condition;
  std::unique_ptr<Body> then;
  std::unique_ptr<Body> elseBody;
  IfStatement(std::unique_ptr<Expression> condition, std::unique_ptr<Body> then,
              std::unique_ptr<Body> elseBody);
  int accept(Visitor *visitor) override;
  ~IfStatement() override;
};

class ForStatement : public Statement {
public:
  ForInitializer initializer;
  std::unique_ptr<Expression> condition;
  std::unique_ptr<Statement> update;
  std::unique_ptr<Body> body;
  ForStatement(ForInitializer initializer,
               std::unique_ptr<Expression> condition,
               std::unique_ptr<Statement> update, std::unique_ptr<Body> body);
  int accept(Visitor *visitor) override;
  ~ForStatement() override;
};

class WhileStatement : public Statement {
public:
  std::unique_ptr<Expression> condition;
  std::unique_ptr<Body> body;
  WhileStatement(std::unique_ptr<Expression> condition,
                 std::unique_ptr<Body> body);
  int accept(Visitor *visitor) override;
  ~WhileStatement() override;
};

class DoWhileStatement : public Statement {
public:
  std::unique_ptr<Body> body;
  std::unique_ptr<Expression> condition;
  DoWhileStatement(std::unique_ptr<Body> body,
                   std::unique_ptr<Expression> condition);
  int accept(Visitor *visitor) override;
  ~DoWhileStatement() override;
};

class BreakStatement : public Statement {
public:
  BreakStatement() = default;
  int accept(Visitor *visitor) override;
  ~BreakStatement() override = default;
};

class ContinueStatement : public Statement {
public:
  ContinueStatement() = default;
  int accept(Visitor *visitor) override;
  ~ContinueStatement() override = default;
};

class CaseStatement {
public:
  std::unique_ptr<Expression> expression;
  std::int64_t value = 0;
  std::list<std::unique_ptr<Statement>> body;
  explicit CaseStatement(std::unique_ptr<Expression> expression);
  ~CaseStatement() = default;
};

class SwitchStatement : public Statement {
public:
  std::unique_ptr<Expression> expression;
  std::list<std::unique_ptr<CaseStatement>> cases;
  std::list<std::unique_ptr<Statement>> defaultBody;
  explicit SwitchStatement(std::unique_ptr<Expression> expression);
  int accept(Visitor *visitor) override;
  ~SwitchStatement() override = default;
};

class VariableDeclaration {
public:
  std::string baseType;
  std::list<Declarator> declarators;
  VariableDeclaration() = default;
  int accept(Visitor *visitor);
  ~VariableDeclaration() = default;
};

class BlockItem {
public:
  virtual ~BlockItem() = default;
};

class DeclarationItem : public BlockItem {
public:
  std::unique_ptr<VariableDeclaration> declaration;
  explicit DeclarationItem(std::unique_ptr<VariableDeclaration> declaration);
  ~DeclarationItem() override = default;
};

class StatementItem : public BlockItem {
public:
  std::unique_ptr<Statement> statement;
  explicit StatementItem(std::unique_ptr<Statement> statement);
  ~StatementItem() override = default;
};

class StructDeclaration {
public:
  std::string name;
  std::list<std::unique_ptr<VariableDeclaration>> fields;
  StructDeclaration() = default;
  int accept(Visitor *visitor);
  ~StructDeclaration() = default;
};

class Body {
public:
  std::list<std::unique_ptr<BlockItem>> items;
  Body() = default;
  int accept(Visitor *visitor);
  ~Body() = default;
};

class FunctionDeclaration {
public:
  std::string name;
  std::string returnBaseType;
  std::size_t returnPointerDepth = 0;
  std::unique_ptr<Body> body;
  std::vector<std::string> parameterBaseTypes;
  std::vector<std::size_t> parameterPointerDepths;
  std::vector<std::string> parameterNames;
  bool variadic = false;
  FunctionDeclaration() = default;
  bool hasBody() const;
  int accept(Visitor *visitor);
  ~FunctionDeclaration() = default;
};

class Program {
public:
  std::list<std::unique_ptr<StructDeclaration>> structDeclarations;
  std::list<std::unique_ptr<VariableDeclaration>> globalDeclarations;
  std::list<std::unique_ptr<FunctionDeclaration>> functionDeclarations;
  Program() = default;
  int accept(Visitor *visitor);
  ~Program() = default;
};

#endif
