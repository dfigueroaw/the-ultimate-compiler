#include "../include/expression_optimizer.h"

#include "../include/visitor_utils.h"

#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>

namespace {
class ExpressionOptimizer {
public:
  explicit ExpressionOptimizer(const SemanticInfo &semantics)
      : semantics(semantics), structInfos(semantics.structs) {}

  void optimize(Program *program) {
    variableTypes.add_level();
    for (auto &declaration : program->globalDeclarations) {
      registerDeclaration(declaration.get());
      foldDeclaration(declaration.get());
    }
    for (auto &function : program->functionDeclarations)
      if (function->hasBody())
        foldFunction(function.get());
    variableTypes.remove_level();
  }

private:
  struct ConstantValue {
    std::int64_t value = 0;
  };

  const SemanticInfo &semantics;
  const std::unordered_map<std::string, StructInfo> &structInfos;
  Environment<TypeInfo> variableTypes;
  std::string currentFunction;

  void foldFunction(FunctionDeclaration *function) {
    currentFunction = function->name;
    variableTypes.add_level();
    const auto &params = semantics.functionParams.at(function->name);
    for (std::size_t i = 0; i < function->parameterNames.size(); ++i)
      variableTypes.add_var(function->parameterNames[i], params[i]);
    foldBody(function->body.get());
    variableTypes.remove_level();
    currentFunction.clear();
  }

  void foldBody(Body *body) {
    variableTypes.add_level();
    for (auto &item : body->items) {
      if (auto *declaration = dynamic_cast<DeclarationItem *>(item.get())) {
        registerDeclaration(declaration->declaration.get());
        foldDeclaration(declaration->declaration.get());
      } else if (auto *statement = dynamic_cast<StatementItem *>(item.get())) {
        foldStatement(statement->statement.get());
      }
    }
    variableTypes.remove_level();
  }

  void registerDeclaration(VariableDeclaration *declaration) {
    for (auto &declarator : declaration->declarators) {
      variableTypes.add_var(
          declarator.name,
          TypeInfo{declaration->baseType, declarator.pointerDepth,
                   constDimensions(declarator.dimensions)});
    }
  }

  void foldDeclaration(VariableDeclaration *declaration) {
    for (auto &declarator : declaration->declarators) {
      for (auto &dimension : declarator.dimensions)
        foldExpression(dimension);
      if (declarator.initializer)
        foldExpression(declarator.initializer);
    }
  }

  void foldStatement(Statement *statement) {
    if (auto *assign = dynamic_cast<AssignmentStatement *>(statement)) {
      foldExpression(assign->target);
      foldExpression(assign->expression);
    } else if (auto *assign =
                   dynamic_cast<CompoundAssignmentStatement *>(statement)) {
      foldExpression(assign->target);
      foldExpression(assign->expression);
    } else if (auto *expr = dynamic_cast<ExpressionStatement *>(statement)) {
      foldExpression(expr->expression);
    } else if (auto *ret = dynamic_cast<ReturnStatement *>(statement)) {
      if (ret->expression)
        foldExpression(ret->expression);
    } else if (auto *ifStm = dynamic_cast<IfStatement *>(statement)) {
      foldExpression(ifStm->condition);
      foldBody(ifStm->then.get());
      if (ifStm->elseBody)
        foldBody(ifStm->elseBody.get());
    } else if (auto *forStm = dynamic_cast<ForStatement *>(statement)) {
      variableTypes.add_level();
      std::visit(
          [&](auto &initializer) {
            using Initializer = std::decay_t<decltype(initializer)>;
            if constexpr (std::is_same_v<Initializer,
                                         std::unique_ptr<VariableDeclaration>>) {
              if (initializer) {
                registerDeclaration(initializer.get());
                foldDeclaration(initializer.get());
              }
            } else if constexpr (std::is_same_v<Initializer,
                                                std::unique_ptr<Statement>>) {
              if (initializer)
                foldStatement(initializer.get());
            }
          },
          forStm->initializer);
      if (forStm->condition)
        foldExpression(forStm->condition);
      if (forStm->update)
        foldStatement(forStm->update.get());
      foldBody(forStm->body.get());
      variableTypes.remove_level();
    } else if (auto *whileStm = dynamic_cast<WhileStatement *>(statement)) {
      foldExpression(whileStm->condition);
      foldBody(whileStm->body.get());
    } else if (auto *doWhile = dynamic_cast<DoWhileStatement *>(statement)) {
      foldBody(doWhile->body.get());
      foldExpression(doWhile->condition);
    } else if (auto *switchStm = dynamic_cast<SwitchStatement *>(statement)) {
      foldExpression(switchStm->expression);
      for (auto &caseStm : switchStm->cases) {
        foldExpression(caseStm->expression);
        for (auto &caseStatement : caseStm->body)
          foldStatement(caseStatement.get());
      }
      for (auto &defaultStatement : switchStm->defaultBody)
        foldStatement(defaultStatement.get());
    }
  }

  void foldExpression(std::unique_ptr<Expression> &expression) {
    if (!expression)
      return;

    if (auto *binary = dynamic_cast<BinaryExpression *>(expression.get())) {
      foldExpression(binary->left);
      foldExpression(binary->right);
    } else if (auto *unary = dynamic_cast<UnaryExpression *>(expression.get())) {
      foldExpression(unary->operand);
    } else if (auto *incDec =
                   dynamic_cast<IncDecExpression *>(expression.get())) {
      foldExpression(incDec->operand);
      return;
    } else if (auto *subscript =
                   dynamic_cast<SubscriptExpression *>(expression.get())) {
      foldExpression(subscript->base);
      for (auto &index : subscript->indices)
        foldExpression(index);
      return;
    } else if (auto *deref =
                   dynamic_cast<DereferenceExpression *>(expression.get())) {
      foldExpression(deref->operand);
      return;
    } else if (auto *address =
                   dynamic_cast<AddressExpression *>(expression.get())) {
      foldExpression(address->operand);
      return;
    } else if (auto *size = dynamic_cast<SizeofExpression *>(expression.get())) {
      if (size->expression)
        foldExpression(size->expression);
    } else if (auto *call =
                   dynamic_cast<FunctionCallExpression *>(expression.get())) {
      for (auto &argument : call->arguments)
        foldExpression(argument);
      return;
    } else if (auto *field =
                   dynamic_cast<FieldExpression *>(expression.get())) {
      foldExpression(field->object);
      return;
    } else {
      return;
    }

    const auto value = constantValue(expression.get());
    if (!value) {
      simplifyExpression(expression);
      return;
    }

    auto replacement = std::make_unique<NumberExpression>(value->value);
    if (!sameExpressionType(expression.get(), replacement.get()))
      return;
    expression = std::move(replacement);
    return;
  }

  void simplifyExpression(std::unique_ptr<Expression> &expression) {
    auto *binary = dynamic_cast<BinaryExpression *>(expression.get());
    if (!binary)
      return;

    if ((binary->op == PLUS_OP || binary->op == MUL_OP) &&
        isLiteral(binary->left.get(), identityValue(binary->op))) {
      replaceWithChildIfSameType(expression, binary->right);
      return;
    }

    if ((binary->op == PLUS_OP || binary->op == MINUS_OP ||
         binary->op == MUL_OP || binary->op == DIV_OP) &&
        isLiteral(binary->right.get(), identityValue(binary->op))) {
      replaceWithChildIfSameType(expression, binary->left);
      return;
    }
  }

  std::optional<ConstantValue> constantValue(Expression *expression) const {
    if (auto *number = dynamic_cast<NumberExpression *>(expression))
      return ConstantValue{number->value};

    if (auto *unary = dynamic_cast<UnaryExpression *>(expression)) {
      const auto inner = constantValue(unary->operand.get());
      if (!inner)
        return std::nullopt;
      if (unary->op == NOT_OP)
        return ConstantValue{inner->value == 0 ? 1 : 0};
      if (unary->op == NEG_OP) {
        if (inner->value == std::numeric_limits<std::int64_t>::min())
          return std::nullopt;
        return ConstantValue{-inner->value};
      }
      return std::nullopt;
    }

    if (auto *binary = dynamic_cast<BinaryExpression *>(expression)) {
      const auto left = constantValue(binary->left.get());
      const auto right = constantValue(binary->right.get());
      if (!left || !right)
        return std::nullopt;
      return binaryValue(binary->op, left->value, right->value);
    }

    if (auto *size = dynamic_cast<SizeofExpression *>(expression))
      return sizeofValue(size);

    return std::nullopt;
  }

  std::int64_t identityValue(BinaryOp op) const {
    return op == MUL_OP || op == DIV_OP ? 1 : 0;
  }

  bool isLiteral(Expression *expression, std::int64_t value) const {
    auto *number = dynamic_cast<NumberExpression *>(expression);
    return number && number->value == value;
  }

  void replaceWithChildIfSameType(std::unique_ptr<Expression> &expression,
                                  std::unique_ptr<Expression> &child) {
    if (!sameExpressionType(expression.get(), child.get()))
      return;
    expression = std::move(child);
  }

  std::optional<ConstantValue> binaryValue(BinaryOp op, std::int64_t left,
                                           std::int64_t right) const {
    std::int64_t result = 0;
    switch (op) {
    case PLUS_OP:
      if (__builtin_add_overflow(left, right, &result))
        return std::nullopt;
      return ConstantValue{result};
    case MINUS_OP:
      if (__builtin_sub_overflow(left, right, &result))
        return std::nullopt;
      return ConstantValue{result};
    case MUL_OP:
      if (__builtin_mul_overflow(left, right, &result))
        return std::nullopt;
      return ConstantValue{result};
    case DIV_OP:
      if (right == 0 ||
          (left == std::numeric_limits<std::int64_t>::min() && right == -1))
        return std::nullopt;
      return ConstantValue{left / right};
    case MOD_OP:
      if (right == 0 ||
          (left == std::numeric_limits<std::int64_t>::min() && right == -1))
        return std::nullopt;
      return ConstantValue{left % right};
    case LE_OP: return ConstantValue{left < right ? 1 : 0};
    case GT_OP: return ConstantValue{left > right ? 1 : 0};
    case LEQ_OP: return ConstantValue{left <= right ? 1 : 0};
    case GEQ_OP: return ConstantValue{left >= right ? 1 : 0};
    case EQ_OP: return ConstantValue{left == right ? 1 : 0};
    case NE_OP: return ConstantValue{left != right ? 1 : 0};
    case AND_OP: return ConstantValue{(left != 0 && right != 0) ? 1 : 0};
    case OR_OP: return ConstantValue{(left != 0 || right != 0) ? 1 : 0};
    }
    return std::nullopt;
  }

  std::optional<ConstantValue> sizeofValue(SizeofExpression *expression) const {
    TypeInfo type;
    if (expression->expression) {
      if (auto *string =
              dynamic_cast<StringExpression *>(expression->expression.get())) {
        type = TypeInfo{"char", 0, {string->value.size() + 1}};
      } else if (isLvalueExpression(expression->expression.get())) {
        type = expressionTypeResolver().lvalue(expression->expression.get());
      } else {
        type = expressionTypeResolver()
                   .expression(expression->expression.get())
                   .decayed();
      }
    } else {
      type = TypeInfo{expression->baseType, expression->pointerDepth, {}};
    }

    return ConstantValue{
        static_cast<std::int64_t>(storageBytes(type, structInfos))};
  }

  TypeResolver expressionTypeResolver() const {
    return TypeResolver(
        [&](const std::string &name) { return variableTypes.lookup(name); },
        [&](const std::string &name) -> std::optional<TypeInfo> {
          if (!semantics.functionReturns.contains(name))
            return std::nullopt;
          return semantics.functionReturns.at(name);
        },
        nullptr, structInfos, currentFunction);
  }

  bool sameExpressionType(Expression *original, Expression *replacement) const {
    auto resolver = expressionTypeResolver();
    const TypeInfo originalType =
        isLvalueExpression(original) ? resolver.lvalue(original).decayed()
                                     : resolver.expression(original).decayed();
    const TypeInfo replacementType = resolver.expression(replacement).decayed();
    return originalType.same(replacementType);
  }
};
} // namespace

void optimizeExpressions(Program *program, const SemanticInfo &semantics) {
  ExpressionOptimizer optimizer(semantics);
  optimizer.optimize(program);
}
