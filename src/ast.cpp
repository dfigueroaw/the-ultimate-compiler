

#include "../include/ast.h"

#include <utility>

BinaryExpression::BinaryExpression(std::unique_ptr<Expression> l,
                                   std::unique_ptr<Expression> r, BinaryOp o)
    : left(std::move(l)), right(std::move(r)), op(o) {}

UnaryExpression::UnaryExpression(std::unique_ptr<Expression> o, UnaryOp unaryOp)
    : operand(std::move(o)), op(unaryOp) {}

IncDecExpression::IncDecExpression(std::unique_ptr<Expression> o, IncDecOp op,
                                   bool isPostfix)
    : operand(std::move(o)), op(op), postfix(isPostfix) {}

NumberExpression::NumberExpression(std::int64_t v) : value(v) {}

StringExpression::StringExpression(std::string v) : value(std::move(v)) {}

IdentifierExpression::IdentifierExpression(std::string v)
    : value(std::move(v)) {}

SubscriptExpression::SubscriptExpression(
    std::unique_ptr<Expression> b, std::vector<std::unique_ptr<Expression>> idx)
    : base(std::move(b)), indices(std::move(idx)) {}

DereferenceExpression::DereferenceExpression(std::unique_ptr<Expression> o)
    : operand(std::move(o)) {}

AddressExpression::AddressExpression(std::unique_ptr<Expression> o)
    : operand(std::move(o)) {}

SizeofExpression::SizeofExpression(std::string t, std::size_t ptrs)
    : baseType(std::move(t)), pointerDepth(ptrs) {}

SizeofExpression::SizeofExpression(std::unique_ptr<Expression> expr)
    : pointerDepth(0), expression(std::move(expr)) {}

FieldExpression::FieldExpression(std::unique_ptr<Expression> o, std::string f,
                                 bool ptr)
    : object(std::move(o)), field(std::move(f)), throughPointer(ptr) {}

AssignmentStatement::AssignmentStatement(std::unique_ptr<Expression> t,
                                         std::unique_ptr<Expression> expr)
    : target(std::move(t)), expression(std::move(expr)) {}

CompoundAssignmentStatement::CompoundAssignmentStatement(
    std::unique_ptr<Expression> t, std::unique_ptr<Expression> expr, BinaryOp o)
    : target(std::move(t)), expression(std::move(expr)), op(o) {}

ExpressionStatement::ExpressionStatement(std::unique_ptr<Expression> expr)
    : expression(std::move(expr)) {}

DeclarationItem::DeclarationItem(std::unique_ptr<VariableDeclaration> dec)
    : declaration(std::move(dec)) {}

StatementItem::StatementItem(std::unique_ptr<Statement> stm)
    : statement(std::move(stm)) {}

IfStatement::IfStatement(std::unique_ptr<Expression> c, std::unique_ptr<Body> t,
                         std::unique_ptr<Body> e)
    : condition(std::move(c)), then(std::move(t)), elseBody(std::move(e)) {}

IfStatement::~IfStatement() = default;

ForStatement::ForStatement(ForInitializer init,
                           std::unique_ptr<Expression> cond,
                           std::unique_ptr<Statement> step,
                           std::unique_ptr<Body> loopBody)
    : initializer(std::move(init)), condition(std::move(cond)),
      update(std::move(step)), body(std::move(loopBody)) {}

ForStatement::~ForStatement() = default;

WhileStatement::WhileStatement(std::unique_ptr<Expression> c,
                               std::unique_ptr<Body> t)
    : condition(std::move(c)), body(std::move(t)) {}

WhileStatement::~WhileStatement() = default;

DoWhileStatement::DoWhileStatement(std::unique_ptr<Body> body,
                                   std::unique_ptr<Expression> cond)
    : body(std::move(body)), condition(std::move(cond)) {}

DoWhileStatement::~DoWhileStatement() = default;

SwitchStatement::SwitchStatement(std::unique_ptr<Expression> expr)
    : expression(std::move(expr)) {}

bool FunctionDeclaration::hasBody() const { return body != nullptr; }
