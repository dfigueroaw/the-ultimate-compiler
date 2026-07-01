#include "../include/visitor_utils.h"

#include <cstdint>
#include <stdexcept>

bool isScalarInteger(const TypeInfo &type) { return type.isScalarInteger(); }

TypeInfo integerLiteralType(const NumberExpression *number) {
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

TypeInfo integerPromotion(const TypeInfo &type) {
  if (!isScalarInteger(type))
    return type;
  if (integerRank(type) < integerRank(TypeInfo{"int", 0, {}}))
    return TypeInfo{"int", 0, {}};
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
  return integerRank(promotedLeft) >= integerRank(promotedRight) ? promotedLeft
                                                                : promotedRight;
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
    if ((isScalarInteger(lhs) && isScalarInteger(rhs)) ||
        areCompatiblePointers(lhs, rhs)) {
      semantics.result = TypeInfo{"int", 0, {}};
      semantics.comparison = true;
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
