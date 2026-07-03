#include "../include/expression_optimizer.h"

#include "../include/visitor_utils.h"

#include <cstddef>
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
    std::uint64_t bits = 0;
    TypeInfo type;
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
      foldExpression(subscript->index);
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

    const std::int64_t replacementValue =
        value->type.isUnsignedInteger()
            ? static_cast<std::int64_t>(value->bits)
            : signedValue(*value);
    auto replacement = std::make_unique<NumberExpression>(replacementValue);
    if (!sameExpressionType(expression.get(), replacement.get()))
      return;
    expression = std::move(replacement);
    return;
  }

  void simplifyExpression(std::unique_ptr<Expression> &expression) {
    auto *binary = dynamic_cast<BinaryExpression *>(expression.get());
    if (!binary)
      return;

    if (binary->op == AND_OP && isLiteral(binary->left.get(), 0)) {
      replaceWithLiteralIfSameType(expression, 0);
      return;
    }

    if (binary->op == OR_OP && isNonzeroLiteral(binary->left.get())) {
      replaceWithLiteralIfSameType(expression, 1);
      return;
    }

    if (binary->op == AND_OP && isNonzeroLiteral(binary->left.get())) {
      replaceWithBooleanComparison(expression, binary->right);
      return;
    }

    if (binary->op == OR_OP && isLiteral(binary->left.get(), 0)) {
      replaceWithBooleanComparison(expression, binary->right);
      return;
    }

    if (binary->op == AND_OP && isNonzeroLiteral(binary->right.get())) {
      replaceWithBooleanComparison(expression, binary->left);
      return;
    }

    if (binary->op == OR_OP && isLiteral(binary->right.get(), 0)) {
      replaceWithBooleanComparison(expression, binary->left);
      return;
    }

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
    if (auto *number = dynamic_cast<NumberExpression *>(expression)) {
      const TypeInfo type = expressionType(number);
      return ConstantValue{convertBits(static_cast<std::uint64_t>(number->value),
                                       type),
                           type};
    }

    if (auto *unary = dynamic_cast<UnaryExpression *>(expression)) {
      const auto inner = constantValue(unary->operand.get());
      if (!inner)
        return std::nullopt;
      const TypeInfo resultType = expressionType(expression);
      if (unary->op == NOT_OP)
        return ConstantValue{inner->bits == 0 ? 1U : 0U, resultType};
      if (unary->op == NEG_OP) {
        if (resultType.isUnsignedInteger())
          return ConstantValue{
              convertBits(std::uint64_t{0} -
                              convertBits(inner->bits, resultType),
                          resultType),
              resultType};
        if (signedValue(*inner) == signedMin(resultType))
          return std::nullopt;
        return ConstantValue{
            convertBits(static_cast<std::uint64_t>(-signedValue(*inner)),
                        resultType),
            resultType};
      }
      return std::nullopt;
    }

    if (auto *binary = dynamic_cast<BinaryExpression *>(expression)) {
      const auto left = constantValue(binary->left.get());
      const auto right = constantValue(binary->right.get());
      if (!left || !right)
        return std::nullopt;
      return binaryValue(binary, *left, *right);
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

  bool isNonzeroLiteral(Expression *expression) const {
    auto *number = dynamic_cast<NumberExpression *>(expression);
    return number && number->value != 0;
  }

  std::size_t integerWidthBits(const TypeInfo &type) const {
    return storageBytes(type, structInfos) * 8;
  }

  std::uint64_t maskForWidth(std::size_t bits) const {
    if (bits >= 64)
      return std::numeric_limits<std::uint64_t>::max();
    return (std::uint64_t{1} << bits) - 1;
  }

  std::uint64_t convertBits(std::uint64_t bits, const TypeInfo &type) const {
    if (!type.isScalarInteger())
      return bits;
    return bits & maskForWidth(integerWidthBits(type));
  }

  std::int64_t signedValue(const ConstantValue &value) const {
    const std::size_t bits = integerWidthBits(value.type);
    const std::uint64_t masked = convertBits(value.bits, value.type);
    if (bits >= 64)
      return static_cast<std::int64_t>(masked);
    const std::uint64_t sign = std::uint64_t{1} << (bits - 1);
    if ((masked & sign) == 0)
      return static_cast<std::int64_t>(masked);
    return static_cast<std::int64_t>(masked | ~maskForWidth(bits));
  }

  std::int64_t signedMax(const TypeInfo &type) const {
    const std::size_t bits = integerWidthBits(type);
    if (bits >= 64)
      return std::numeric_limits<std::int64_t>::max();
    return static_cast<std::int64_t>((std::uint64_t{1} << (bits - 1)) - 1);
  }

  std::int64_t signedMin(const TypeInfo &type) const {
    if (integerWidthBits(type) >= 64)
      return std::numeric_limits<std::int64_t>::min();
    return -signedMax(type) - 1;
  }

  bool signedFits(std::int64_t value, const TypeInfo &type) const {
    return value >= signedMin(type) && value <= signedMax(type);
  }

  ConstantValue converted(ConstantValue value, const TypeInfo &type) const {
    if (value.type.isUnsignedInteger()) {
      value.bits = convertBits(value.bits, type);
    } else {
      value.bits = convertBits(static_cast<std::uint64_t>(signedValue(value)),
                               type);
    }
    value.type = type;
    return value;
  }

  void replaceWithLiteralIfSameType(std::unique_ptr<Expression> &expression,
                                    std::int64_t value) {
    auto replacement = std::make_unique<NumberExpression>(value);
    if (!sameExpressionType(expression.get(), replacement.get()))
      return;
    expression = std::move(replacement);
  }

  void replaceWithChildIfSameType(std::unique_ptr<Expression> &expression,
                                  std::unique_ptr<Expression> &child) {
    if (!sameExpressionType(expression.get(), child.get()))
      return;
    expression = std::move(child);
  }

  void replaceWithBooleanComparison(std::unique_ptr<Expression> &expression,
                                    std::unique_ptr<Expression> &child) {
    const TypeInfo originalType = expressionType(expression.get());
    const TypeInfo childType = expressionType(child.get());
    const TypeInfo zeroType = TypeInfo{"int", 0, {}};
    const TypeInfo replacementType =
        analyzeBinary(NE_OP, childType, zeroType, false, true).result;
    if (!originalType.same(replacementType.decayed()))
      return;
    auto replacement = std::make_unique<BinaryExpression>(
        std::move(child), std::make_unique<NumberExpression>(0), NE_OP);
    expression = std::move(replacement);
  }

  std::optional<ConstantValue> binaryValue(BinaryExpression *binary,
                                           ConstantValue left,
                                           ConstantValue right) const {
    const BinaryOp op = binary->op;
    TypeInfo resultType = expressionType(binary);
    TypeInfo operandType = resultType;
    if (op == LE_OP || op == GT_OP || op == LEQ_OP || op == GEQ_OP ||
        op == EQ_OP || op == NE_OP)
      operandType = usualArithmeticType(left.type, right.type);
    if (op == AND_OP || op == OR_OP)
      operandType = TypeInfo{"int", 0, {}};

    left = converted(left, operandType);
    right = converted(right, operandType);
    const bool isUnsigned = operandType.isUnsignedInteger();
    const std::uint64_t leftUnsigned = convertBits(left.bits, operandType);
    const std::uint64_t rightUnsigned = convertBits(right.bits, operandType);
    const std::int64_t leftSigned = signedValue(left);
    const std::int64_t rightSigned = signedValue(right);

    auto makeResult = [&](std::uint64_t bits) {
      return ConstantValue{convertBits(bits, resultType), resultType};
    };

    std::int64_t signedResult = 0;
    switch (op) {
    case PLUS_OP:
      if (isUnsigned)
        return makeResult(leftUnsigned + rightUnsigned);
      if (__builtin_add_overflow(leftSigned, rightSigned, &signedResult))
        return std::nullopt;
      if (!signedFits(signedResult, resultType))
        return std::nullopt;
      return makeResult(static_cast<std::uint64_t>(signedResult));
    case MINUS_OP:
      if (isUnsigned)
        return makeResult(leftUnsigned - rightUnsigned);
      if (__builtin_sub_overflow(leftSigned, rightSigned, &signedResult))
        return std::nullopt;
      if (!signedFits(signedResult, resultType))
        return std::nullopt;
      return makeResult(static_cast<std::uint64_t>(signedResult));
    case MUL_OP:
      if (isUnsigned)
        return makeResult(leftUnsigned * rightUnsigned);
      if (__builtin_mul_overflow(leftSigned, rightSigned, &signedResult))
        return std::nullopt;
      if (!signedFits(signedResult, resultType))
        return std::nullopt;
      return makeResult(static_cast<std::uint64_t>(signedResult));
    case DIV_OP:
      if ((isUnsigned && rightUnsigned == 0) ||
          (!isUnsigned &&
           (rightSigned == 0 ||
            (leftSigned == signedMin(operandType) && rightSigned == -1))))
        return std::nullopt;
      return isUnsigned ? makeResult(leftUnsigned / rightUnsigned)
                        : makeResult(static_cast<std::uint64_t>(leftSigned /
                                                                rightSigned));
    case MOD_OP:
      if ((isUnsigned && rightUnsigned == 0) ||
          (!isUnsigned &&
           (rightSigned == 0 ||
            (leftSigned == signedMin(operandType) && rightSigned == -1))))
        return std::nullopt;
      return isUnsigned ? makeResult(leftUnsigned % rightUnsigned)
                        : makeResult(static_cast<std::uint64_t>(leftSigned %
                                                                rightSigned));
    case LE_OP:
      return makeResult(isUnsigned ? leftUnsigned < rightUnsigned
                                   : leftSigned < rightSigned);
    case GT_OP:
      return makeResult(isUnsigned ? leftUnsigned > rightUnsigned
                                   : leftSigned > rightSigned);
    case LEQ_OP:
      return makeResult(isUnsigned ? leftUnsigned <= rightUnsigned
                                   : leftSigned <= rightSigned);
    case GEQ_OP:
      return makeResult(isUnsigned ? leftUnsigned >= rightUnsigned
                                   : leftSigned >= rightSigned);
    case EQ_OP: return makeResult(leftUnsigned == rightUnsigned);
    case NE_OP: return makeResult(leftUnsigned != rightUnsigned);
    case AND_OP:
      return makeResult((left.bits != 0 && right.bits != 0) ? 1 : 0);
    case OR_OP:
      return makeResult((left.bits != 0 || right.bits != 0) ? 1 : 0);
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

    const TypeInfo resultType{"int", 0, {}};
    return ConstantValue{storageBytes(type, structInfos), resultType};
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
    return expressionType(original).same(expressionType(replacement));
  }

  TypeInfo expressionType(Expression *expression) const {
    auto resolver = expressionTypeResolver();
    if (isLvalueExpression(expression))
      return resolver.lvalue(expression).decayed();
    return resolver.expression(expression).decayed();
  }
};
} // namespace

void optimizeExpressions(Program *program, const SemanticInfo &semantics) {
  ExpressionOptimizer optimizer(semantics);
  optimizer.optimize(program);
}
