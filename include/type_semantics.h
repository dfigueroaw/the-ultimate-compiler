#ifndef TYPE_SEMANTICS_H
#define TYPE_SEMANTICS_H

#include "visitor.h"

enum class ConversionKind {
  Identity,
  Integer,
  Pointer,
  NullPointer,
  Struct,
  Incompatible
};

struct Conversion {
  ConversionKind kind = ConversionKind::Incompatible;
  TypeInfo source;
  TypeInfo target;

  bool valid() const { return kind != ConversionKind::Incompatible; }
};

bool isScalarInteger(const TypeInfo &type);
TypeInfo integerLiteralType(const NumberExpression *number);
int integerRank(const TypeInfo &type);
TypeInfo integerPromotion(const TypeInfo &type);
TypeInfo defaultArgumentPromotion(const TypeInfo &type);
TypeInfo usualArithmeticType(const TypeInfo &left, const TypeInfo &right);
bool isPointerType(const TypeInfo &type);
bool isVoidPointer(const TypeInfo &type);
bool areCompatiblePointers(const TypeInfo &left, const TypeInfo &right);
TypeInfo pointedType(TypeInfo type);

Conversion assignmentConversion(const TypeInfo &target, const TypeInfo &source,
                                bool sourceNullPointerConstant = false);
Conversion conditionConversion(const TypeInfo &source);

struct BinarySemantics {
  TypeInfo result;
  bool integerArithmetic = false;
  bool pointerArithmetic = false;
  bool pointerDifference = false;
  bool comparison = false;
  bool logical = false;
};

BinarySemantics analyzeBinary(BinaryOp op, const TypeInfo &left,
                              const TypeInfo &right,
                              bool leftNullPointerConstant = false,
                              bool rightNullPointerConstant = false);

#endif
