#include "../include/visitor_utils.h"

#include <cstddef>
#include <cstdint>
#include <stdexcept>

bool isScalarInteger(const TypeInfo &type) { return type.isScalarInteger(); }

bool isUnsignedInteger(const TypeInfo &type) {
  return type.isUnsignedInteger();
}

TypeInfo integerLiteralType(const NumberExpression *number) {
  if (number->typeName)
    return TypeInfo{*number->typeName, 0, {}};
  const auto maxInt = static_cast<std::int64_t>(
      (std::uint64_t{1} << (targetInfo().intSize * 8 - 1)) - 1);
  if (number->value <= maxInt)
    return TypeInfo{"int", 0, {}};
  return TypeInfo{"long", 0, {}};
}

int integerRank(const TypeInfo &type) {
  if (type.kind == TypeKind::Char)
    return 1;
  if (type.kind == TypeKind::Short)
    return 2;
  if (type.kind == TypeKind::Int)
    return 3;
  if (type.kind == TypeKind::Long)
    return 4;
  if (type.kind == TypeKind::LongLong)
    return 5;
  throw std::runtime_error("[Type] Tipo no entero sin rango");
}

namespace {
std::size_t integerBytes(const TypeInfo &type) {
  const auto &info = targetInfo();
  if (type.kind == TypeKind::Char)
    return info.charSize;
  if (type.kind == TypeKind::Short)
    return info.shortSize;
  if (type.kind == TypeKind::Int)
    return info.intSize;
  if (type.kind == TypeKind::Long)
    return info.longSize;
  if (type.kind == TypeKind::LongLong)
    return info.longLongSize;
  throw std::runtime_error("[Type] Tipo no entero sin tamaño");
}

bool signedCanRepresentAllValuesOf(const TypeInfo &signedType,
                                   const TypeInfo &unsignedType) {
  return !signedType.isUnsignedInteger() && unsignedType.isUnsignedInteger() &&
         integerBytes(signedType) > integerBytes(unsignedType);
}
} // namespace

TypeInfo signedVersion(TypeInfo type) {
  if (type.isScalarInteger())
    type.isUnsigned = false;
  return type;
}

TypeInfo unsignedVersion(TypeInfo type) {
  if (type.isScalarInteger())
    type.isUnsigned = true;
  return type;
}

TypeInfo integerPromotion(const TypeInfo &type) {
  if (!isScalarInteger(type))
    return type;
  if (integerRank(type) < integerRank(TypeInfo{"int", 0, {}})) {
    if (type.isUnsignedInteger() &&
        integerBytes(type) >= integerBytes(TypeInfo{"int", 0, {}}))
      return TypeInfo{"unsigned int", 0, {}};
    return TypeInfo{"int", 0, {}};
  }
  return type;
}

TypeInfo defaultArgumentPromotion(const TypeInfo &type) {
  return integerPromotion(type.decayed());
}

TypeInfo usualArithmeticType(const TypeInfo &left, const TypeInfo &right) {
  const TypeInfo promotedLeft = integerPromotion(left.decayed());
  const TypeInfo promotedRight = integerPromotion(right.decayed());
  if (!isScalarInteger(promotedLeft) || !isScalarInteger(promotedRight))
    throw std::runtime_error("[Type] Conversión aritmética requiere enteros");
  if (promotedLeft.isUnsignedInteger() == promotedRight.isUnsignedInteger())
    return integerRank(promotedLeft) >= integerRank(promotedRight)
               ? promotedLeft
               : promotedRight;

  const TypeInfo unsignedType =
      promotedLeft.isUnsignedInteger() ? promotedLeft : promotedRight;
  const TypeInfo signedType =
      promotedLeft.isUnsignedInteger() ? promotedRight : promotedLeft;
  if (integerRank(unsignedType) >= integerRank(signedType))
    return unsignedType;
  if (signedCanRepresentAllValuesOf(signedType, unsignedType))
    return signedType;
  return unsignedVersion(signedType);
}

bool isPointerType(const TypeInfo &type) { return type.isPointer(); }

bool isVoidPointer(const TypeInfo &type) {
  return type.kind == TypeKind::Pointer &&
         type.pointee().kind == TypeKind::Void;
}

bool areCompatiblePointers(const TypeInfo &left, const TypeInfo &right) {
  if (!isPointerType(left) || !isPointerType(right))
    return false;
  return left.same(right) || isVoidPointer(left) || isVoidPointer(right);
}

TypeInfo pointedType(TypeInfo type) {
  if (type.kind == TypeKind::Array || type.kind == TypeKind::Pointer)
    return type.pointee();
  return type;
}

Conversion assignmentConversion(const TypeInfo &target, const TypeInfo &source,
                                bool sourceNullPointerConstant) {
  const TypeInfo lhs = target.decayed();
  const TypeInfo rhs = source.decayed();
  Conversion conversion;
  conversion.source = rhs;
  conversion.target = lhs;

  if (lhs.same(rhs)) {
    conversion.kind = ConversionKind::Identity;
    return conversion;
  }

  if (isScalarInteger(lhs) && isScalarInteger(rhs)) {
    conversion.kind = ConversionKind::Integer;
    return conversion;
  }

  if (lhs.isStructObject() && rhs.isStructObject() && lhs.same(rhs)) {
    conversion.kind = ConversionKind::Struct;
    return conversion;
  }

  if (isPointerType(lhs) && isPointerType(rhs) &&
      areCompatiblePointers(lhs, rhs)) {
    conversion.kind = ConversionKind::Pointer;
    return conversion;
  }

  if (isPointerType(lhs) && isScalarInteger(rhs) &&
      sourceNullPointerConstant) {
    conversion.kind = ConversionKind::NullPointer;
    return conversion;
  }

  return conversion;
}

Conversion conditionConversion(const TypeInfo &source) {
  const TypeInfo type = source.decayed();
  Conversion conversion;
  conversion.source = type;
  conversion.target = TypeInfo{"int", 0, {}};
  if (isScalarInteger(type) || isPointerType(type))
    conversion.kind = ConversionKind::Integer;
  return conversion;
}

BinarySemantics analyzeBinary(BinaryOp op, const TypeInfo &left,
                              const TypeInfo &right,
                              bool leftNullPointerConstant,
                              bool rightNullPointerConstant) {
  const TypeInfo lhs = left.decayed();
  const TypeInfo rhs = right.decayed();
  BinarySemantics semantics;

  if (op == AND_OP || op == OR_OP) {
    if (!conditionConversion(lhs).valid() || !conditionConversion(rhs).valid())
      throw std::runtime_error(
          "[TypeChecker] Operación lógica requiere operandos int o puntero");
    semantics.result = TypeInfo{"int", 0, {}};
    semantics.logical = true;
    return semantics;
  }

  if (op == EQ_OP || op == NE_OP) {
    const bool integerComparison = isScalarInteger(lhs) && isScalarInteger(rhs);
    const bool pointerComparison = areCompatiblePointers(lhs, rhs);
    const bool nullPointerComparison =
        (isPointerType(lhs) && isScalarInteger(rhs) &&
         rightNullPointerConstant) ||
        (isScalarInteger(lhs) && isPointerType(rhs) && leftNullPointerConstant);
    if (!integerComparison && !pointerComparison && !nullPointerComparison)
      throw std::runtime_error(
          "[TypeChecker] Comparación requiere operandos compatibles");
    semantics.result = TypeInfo{"int", 0, {}};
    semantics.comparison = true;
    return semantics;
  }

  if (op == LE_OP || op == GT_OP || op == LEQ_OP || op == GEQ_OP) {
    if (isScalarInteger(lhs) && isScalarInteger(rhs)) {
      const TypeInfo comparisonType = usualArithmeticType(lhs, rhs);
      semantics.result = TypeInfo{"int", 0, {}};
      semantics.comparison = true;
      semantics.unsignedComparison = comparisonType.isUnsignedInteger();
      return semantics;
    }
    if (areCompatiblePointers(lhs, rhs)) {
      semantics.result = TypeInfo{"int", 0, {}};
      semantics.comparison = true;
      semantics.unsignedComparison = true;
      return semantics;
    }
    throw std::runtime_error(
        "[TypeChecker] Comparación relacional requiere enteros o punteros "
        "compatibles");
  }

  if (op == PLUS_OP) {
    if (isScalarInteger(lhs) && isScalarInteger(rhs)) {
      semantics.result = usualArithmeticType(lhs, rhs);
      semantics.integerArithmetic = true;
      return semantics;
    }
    if (isPointerType(lhs) && isScalarInteger(rhs) && !isVoidPointer(lhs)) {
      semantics.result = lhs;
      semantics.pointerArithmetic = true;
      return semantics;
    }
    if (isScalarInteger(lhs) && isPointerType(rhs) && !isVoidPointer(rhs)) {
      semantics.result = rhs;
      semantics.pointerArithmetic = true;
      return semantics;
    }
    throw std::runtime_error(
        "[TypeChecker] Suma requiere enteros o puntero más entero");
  }

  if (op == MINUS_OP) {
    if (isScalarInteger(lhs) && isScalarInteger(rhs)) {
      semantics.result = usualArithmeticType(lhs, rhs);
      semantics.integerArithmetic = true;
      return semantics;
    }
    if (isPointerType(lhs) && isScalarInteger(rhs) && !isVoidPointer(lhs)) {
      semantics.result = lhs;
      semantics.pointerArithmetic = true;
      return semantics;
    }
    if (areCompatiblePointers(lhs, rhs) && !isVoidPointer(lhs) &&
        !isVoidPointer(rhs)) {
      semantics.result = TypeInfo{"int", 0, {}};
      semantics.pointerDifference = true;
      return semantics;
    }
    throw std::runtime_error(
        "[TypeChecker] Resta requiere enteros, puntero menos entero o "
        "punteros compatibles");
  }

  if (!isScalarInteger(lhs) || !isScalarInteger(rhs))
    throw std::runtime_error(
        "[TypeChecker] Operación binaria requiere operandos int");
  semantics.result = usualArithmeticType(lhs, rhs);
  semantics.integerArithmetic = true;
  return semantics;
}

std::int64_t evaluateConstantInt(
    Expression *exp,
    const std::unordered_map<std::string, StructInfo> *structInfos) {
  if (auto *num = dynamic_cast<NumberExpression *>(exp))
    return num->value;

  if (auto *unary = dynamic_cast<UnaryExpression *>(exp)) {
    const auto inner = evaluateConstantInt(unary->operand.get(), structInfos);
    if (unary->op == NEG_OP)
      return -inner;
    if (unary->op == NOT_OP)
      return !inner;
    throw std::runtime_error(
        "[Constant] Operador unario no soportado en expresión constante");
  }

  if (auto *binary = dynamic_cast<BinaryExpression *>(exp)) {
    const auto left = evaluateConstantInt(binary->left.get(), structInfos);
    const auto right = evaluateConstantInt(binary->right.get(), structInfos);
    switch (binary->op) {
    case PLUS_OP: return left + right;
    case MINUS_OP: return left - right;
    case MUL_OP: return left * right;
    case DIV_OP:
      if (right == 0)
        throw std::runtime_error(
            "[Constant] División por cero en expresión constante");
      return left / right;
    case MOD_OP:
      if (right == 0)
        throw std::runtime_error(
            "[Constant] Módulo por cero en expresión constante");
      return left % right;
    case AND_OP: return left && right;
    case OR_OP: return left || right;
    case EQ_OP: return left == right;
    case NE_OP: return left != right;
    case LE_OP: return left < right;
    case GT_OP: return left > right;
    case LEQ_OP: return left <= right;
    case GEQ_OP: return left >= right;
    }
  }

  if (auto *size = dynamic_cast<SizeofExpression *>(exp)) {
    if (size->expression)
      throw std::runtime_error(
          "[Constant] sizeof de expresión no soportado en expresión "
          "constante");
    const TypeInfo type{size->baseType, size->pointerDepth, {}};
    if (structInfos)
      return static_cast<std::int64_t>(storageBytes(type, *structInfos));
    const auto &info = targetInfo();
    if (size->baseType == "void" && size->pointerDepth > 0)
      return static_cast<std::int64_t>(info.pointerSize);
    if (size->baseType == "char" || size->baseType == "unsigned char")
      return static_cast<std::int64_t>(info.charSize);
    if (size->baseType == "short" || size->baseType == "unsigned short")
      return static_cast<std::int64_t>(info.shortSize);
    if (size->baseType == "int" || size->baseType == "unsigned int")
      return static_cast<std::int64_t>(info.intSize);
    if (size->baseType == "long" || size->baseType == "unsigned long")
      return static_cast<std::int64_t>(info.longSize);
    if (size->baseType == "long long" ||
        size->baseType == "unsigned long long")
      return static_cast<std::int64_t>(info.longLongSize);
    throw std::runtime_error(
        "[Constant] sizeof de tipo desconocido en expresión constante");
  }

  throw std::runtime_error(
      "[Constant] La expresión no es una constante de tiempo de "
      "compilación");
}
