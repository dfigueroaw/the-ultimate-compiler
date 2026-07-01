#include "../include/visitor_utils.h"

#include <stdexcept>
#include <utility>

const TargetInfo &targetInfo() {
  static const TargetInfo info;
  return info;
}

bool isBuiltinType(const std::string &type) {
  return type == "int" || type == "void" || type == "char" ||
         type == "short" || type == "long" || type == "long long";
}

std::optional<std::size_t> constSize(Expression *exp) {
  try {
    const auto value = evaluateConstantInt(exp);
    if (value <= 0)
      return std::nullopt;
    return static_cast<std::size_t>(value);
  } catch (...) {
    return std::nullopt;
  }
}

std::vector<std::size_t>
constDimensions(const std::vector<std::unique_ptr<Expression>> &dimensions) {
  std::vector<std::size_t> result;
  for (const auto &dimension : dimensions) {
    auto size = constSize(dimension.get());
    if (!size)
      throw std::runtime_error(
          "[TypeChecker] Las dimensiones de arreglos estáticos deben ser "
          "constantes positivas");
    result.push_back(*size);
  }
  return result;
}

std::size_t alignTo(std::size_t value, std::size_t align) {
  const std::size_t remainder = value % align;
  if (remainder == 0)
    return value;
  return value + align - remainder;
}

TypeInfo scalarTypeFromName(const std::string &name) {
  if (name == "void")
    return TypeInfo::scalar(TypeKind::Void);
  if (name == "char")
    return TypeInfo::scalar(TypeKind::Char);
  if (name == "short")
    return TypeInfo::scalar(TypeKind::Short);
  if (name == "int")
    return TypeInfo::scalar(TypeKind::Int);
  if (name == "long")
    return TypeInfo::scalar(TypeKind::Long);
  if (name == "long long")
    return TypeInfo::scalar(TypeKind::LongLong);
  return TypeInfo::structure(name);
}

TypeLayout
layoutOf(const TypeInfo &type,
         const std::unordered_map<std::string, StructInfo> &structInfos) {
  const auto &target = targetInfo();
  if (type.kind == TypeKind::Pointer)
    return TypeLayout{target.pointerSize, target.pointerAlign};
  if (type.kind == TypeKind::Array) {
    const auto elementLayout = layoutOf(type.pointee(), structInfos);
    return TypeLayout{elementLayout.size * type.arraySize, elementLayout.align};
  }
  if (type.kind == TypeKind::Struct) {
    if (const auto found = structInfos.find(type.name);
        found != structInfos.end())
      return TypeLayout{found->second.size, found->second.align};
  }
  if (type.kind == TypeKind::Char)
    return TypeLayout{target.charSize, target.charAlign};
  if (type.kind == TypeKind::Short)
    return TypeLayout{target.shortSize, target.shortAlign};
  if (type.kind == TypeKind::Int)
    return TypeLayout{target.intSize, target.intAlign};
  if (type.kind == TypeKind::Long)
    return TypeLayout{target.longSize, target.longAlign};
  if (type.kind == TypeKind::LongLong)
    return TypeLayout{target.longLongSize, target.longLongAlign};
  if (type.kind == TypeKind::Void)
    return TypeLayout{0, 1};
  throw std::runtime_error("[Type] Layout no definido para tipo '" +
                           type.display() + "'");
}

std::size_t
storageBytes(const TypeInfo &type,
             const std::unordered_map<std::string, StructInfo> &structInfos) {
  return layoutOf(type, structInfos).size;
}

bool isVoidObject(const TypeInfo &type) { return type.isVoidObject(); }

bool isNullPointerConstant(Expression *exp) {
  auto *number = dynamic_cast<NumberExpression *>(exp);
  return number && number->value == 0;
}

bool isSizedType(const TypeInfo &type) { return !type.isVoidObject(); }

bool isLvalueExpression(Expression *exp) {
  return dynamic_cast<IdentifierExpression *>(exp) ||
         dynamic_cast<SubscriptExpression *>(exp) ||
         dynamic_cast<DereferenceExpression *>(exp) ||
         dynamic_cast<FieldExpression *>(exp);
}

TypeResolver::TypeResolver(
    VariableLookup variableLookup, FunctionLookup functionLookup,
    FunctionValidator functionValidator,
    const std::unordered_map<std::string, StructInfo> &structInfos,
    std::string currentFunction)
    : variableLookup(std::move(variableLookup)),
      functionLookup(std::move(functionLookup)),
      functionValidator(std::move(functionValidator)), structInfos(structInfos),
      currentFunction(std::move(currentFunction)) {}

TypeInfo TypeResolver::lvalue(Expression *exp) const {
  if (auto *id = dynamic_cast<IdentifierExpression *>(exp)) {
    auto type = variableLookup(id->value);
    if (!type)
      throw std::runtime_error("[TypeChecker] Variable no declarada: '" +
                               id->value + "' usada en la función '" +
                               currentFunction + "'");
    return *type;
  }

  if (auto *subscript = dynamic_cast<SubscriptExpression *>(exp)) {
    TypeInfo result;
    if (isLvalueExpression(subscript->base.get())) {
      result = lvalue(subscript->base.get());
    } else {
      result = expression(subscript->base.get()).decayed();
    }
    for (auto &index : subscript->indices) {
      const auto indexType = expression(index.get()).decayed();
      if (!isScalarInteger(indexType))
        throw std::runtime_error(
            "[TypeChecker] Índice de arreglo debe ser entero");
      if (result.kind == TypeKind::Array || result.kind == TypeKind::Pointer) {
        result = pointedType(result);
      } else {
        throw std::runtime_error(
            "[TypeChecker] Demasiados índices para expresión");
      }
    }
    return result;
  }

  if (auto *deref = dynamic_cast<DereferenceExpression *>(exp)) {
    auto type = expression(deref->operand.get()).decayed();
    if (!isPointerType(type))
      throw std::runtime_error(
          "[TypeChecker] No se puede desreferenciar un valor no puntero");
    return pointedType(type);
  }

  if (auto *field = dynamic_cast<FieldExpression *>(exp)) {
    TypeInfo type = field->throughPointer ? expression(field->object.get()).decayed()
                                          : lvalue(field->object.get());
    if (field->throughPointer) {
      if (!isPointerType(type))
        throw std::runtime_error(
            "[TypeChecker] Operador '->' requiere puntero a estructura");
      type = pointedType(type);
    } else if (isPointerType(type)) {
      throw std::runtime_error(
          "[TypeChecker] Operador '.' requiere objeto estructura");
    }
    const bool validStruct = type.kind == TypeKind::Struct && !type.isArray() &&
                             structInfos.count(type.name);
    if (!validStruct)
      throw std::runtime_error(
          "[TypeChecker] La expresión no tiene el tipo requerido para acceso a "
          "campo");
    if (!structInfos.at(type.name).fields.count(field->field))
      throw std::runtime_error("[TypeChecker] La estructura '" + type.name +
                               "' no tiene campo '" + field->field + "'");
    return structInfos.at(type.name).fields.at(field->field).type;
  }

  throw std::runtime_error("[TypeChecker] La expresión no es asignable");
}

TypeInfo TypeResolver::expression(Expression *exp) const {
  if (auto *number = dynamic_cast<NumberExpression *>(exp))
    return integerLiteralType(number);
  if (auto *string = dynamic_cast<StringExpression *>(exp))
    return TypeInfo{"char", 0, {string->value.size() + 1}};
  if (auto *id = dynamic_cast<IdentifierExpression *>(exp))
    return lvalue(id).decayed();
  if (auto *subscript = dynamic_cast<SubscriptExpression *>(exp))
    return lvalue(subscript).decayed();
  if (auto *deref = dynamic_cast<DereferenceExpression *>(exp))
    return lvalue(deref).decayed();
  if (auto *address = dynamic_cast<AddressExpression *>(exp))
    return TypeInfo::pointerTo(lvalue(address->operand.get()));
  if (auto *size = dynamic_cast<SizeofExpression *>(exp)) {
    if (size->expression) {
      const auto type = isLvalueExpression(size->expression.get())
                            ? lvalue(size->expression.get())
                            : expression(size->expression.get()).decayed();
      if (!isSizedType(type))
        throw std::runtime_error(
            "[TypeChecker] sizeof no acepta expresión void");
    } else {
      if (!isBuiltinType(size->baseType) && !structInfos.count(size->baseType))
        throw std::runtime_error("[TypeChecker] Tipo no definido '" +
                                 size->baseType + "' en expresión sizeof");
      if (size->baseType == "void" && size->pointerDepth == 0)
        throw std::runtime_error("[TypeChecker] sizeof no acepta tipo void");
    }
    return TypeInfo{"int", 0, {}};
  }
  if (auto *call = dynamic_cast<FunctionCallExpression *>(exp)) {
    if (functionValidator)
      functionValidator(call);
    auto type = functionLookup(call->name);
    if (!type)
      throw std::runtime_error("[TypeChecker] Función no definida: '" +
                               call->name + "' llamada en '" +
                               currentFunction + "'");
    return *type;
  }
  if (auto *incDec = dynamic_cast<IncDecExpression *>(exp)) {
    const auto operandType = lvalue(incDec->operand.get()).decayed();
    if (isPointerType(operandType)) {
      if (isVoidPointer(operandType))
        throw std::runtime_error(
            "[TypeChecker] ++/-- no acepta punteros a void");
      return operandType;
    }
    if (isScalarInteger(operandType))
      return operandType;
    throw std::runtime_error(
        "[TypeChecker] ++/-- requiere lvalue entero o puntero");
  }
  if (auto *binary = dynamic_cast<BinaryExpression *>(exp)) {
    const auto left = expression(binary->left.get()).decayed();
    const auto right = expression(binary->right.get()).decayed();
    return analyzeBinary(binary->op, left, right,
                         isNullPointerConstant(binary->left.get()),
                         isNullPointerConstant(binary->right.get()))
        .result;
  }
  if (auto *unary = dynamic_cast<UnaryExpression *>(exp)) {
    const auto operand = expression(unary->operand.get()).decayed();
    if (unary->op == NOT_OP) {
      if (!isScalarInteger(operand) && !isPointerType(operand))
        throw std::runtime_error(
            "[TypeChecker] Operación ! requiere operando int o puntero");
      return TypeInfo{"int", 0, {}};
    }
    if (!isScalarInteger(operand))
      throw std::runtime_error(
          "[TypeChecker] Operación - unaria requiere operando int");
    return integerPromotion(operand);
  }
  if (auto *field = dynamic_cast<FieldExpression *>(exp))
    return lvalue(field).decayed();

  throw std::runtime_error(
      "[TypeChecker] No se pudo inferir tipo de expresión");
}

TypeInfo::TypeInfo(std::string baseName, std::size_t pointerCount,
                   std::vector<std::size_t> dimensions) {
  TypeInfo current = scalarTypeFromName(baseName);
  for (std::size_t i = 0; i < pointerCount; ++i)
    current = pointerTo(current);
  for (auto it = dimensions.rbegin(); it != dimensions.rend(); ++it)
    current = arrayOf(*it, current);
  *this = current;
}

TypeInfo TypeInfo::scalar(TypeKind k) {
  TypeInfo type;
  type.kind = k;
  return type;
}

TypeInfo TypeInfo::structure(std::string structName) {
  TypeInfo type;
  type.kind = TypeKind::Struct;
  type.name = std::move(structName);
  return type;
}

TypeInfo TypeInfo::pointerTo(TypeInfo pointee) {
  TypeInfo type;
  type.kind = TypeKind::Pointer;
  type.element = std::make_shared<TypeInfo>(std::move(pointee));
  return type;
}

TypeInfo TypeInfo::arrayOf(std::size_t size, TypeInfo elem) {
  TypeInfo type;
  type.kind = TypeKind::Array;
  type.arraySize = size;
  type.element = std::make_shared<TypeInfo>(std::move(elem));
  return type;
}

bool TypeInfo::isScalarInteger() const {
  return kind == TypeKind::Int || kind == TypeKind::Char ||
         kind == TypeKind::Short || kind == TypeKind::Long ||
         kind == TypeKind::LongLong;
}

bool TypeInfo::isVoidObject() const { return kind == TypeKind::Void; }

bool TypeInfo::isStructObject() const { return kind == TypeKind::Struct; }

bool TypeInfo::isCharObject() const { return kind == TypeKind::Char; }

bool TypeInfo::isShortObject() const { return kind == TypeKind::Short; }

bool TypeInfo::isIntObject() const { return kind == TypeKind::Int; }

bool TypeInfo::isLongObject() const { return kind == TypeKind::Long; }

bool TypeInfo::isLongLongObject() const { return kind == TypeKind::LongLong; }

const TypeInfo &TypeInfo::pointee() const {
  if (!element)
    throw std::runtime_error("[Type] Tipo sin elemento");
  return *element;
}

TypeInfo TypeInfo::decayed() const {
  if (kind == TypeKind::Array)
    return pointerTo(*element);
  return *this;
}

bool TypeInfo::same(const TypeInfo &other) const {
  if (kind != other.kind)
    return false;
  if (kind == TypeKind::Struct)
    return name == other.name;
  if (kind == TypeKind::Pointer)
    return element->same(*other.element);
  if (kind == TypeKind::Array)
    return arraySize == other.arraySize && element->same(*other.element);
  return true;
}

bool TypeInfo::sameAfterDecay(const TypeInfo &other) const {
  return decayed().same(other.decayed());
}

std::vector<std::size_t> TypeInfo::dimensions() const {
  std::vector<std::size_t> result;
  const TypeInfo *current = this;
  while (current->kind == TypeKind::Array) {
    result.push_back(current->arraySize);
    current = &current->pointee();
  }
  return result;
}

std::string TypeInfo::display() const {
  if (kind == TypeKind::Void)
    return "void";
  if (kind == TypeKind::Char)
    return "char";
  if (kind == TypeKind::Short)
    return "short";
  if (kind == TypeKind::Int)
    return "int";
  if (kind == TypeKind::Long)
    return "long";
  if (kind == TypeKind::LongLong)
    return "long long";
  if (kind == TypeKind::Struct)
    return name;
  if (kind == TypeKind::Pointer)
    return element->display() + "*";
  return element->display() + "[" + std::to_string(arraySize) + "]";
}
