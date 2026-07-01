#ifndef VISITOR_UTILS_H
#define VISITOR_UTILS_H

#include "type_semantics.h"
#include "visitor.h"

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct TargetInfo {
  std::size_t charSize = 1;
  std::size_t charAlign = 1;
  std::size_t shortSize = 2;
  std::size_t shortAlign = 2;
  std::size_t intSize = 4;
  std::size_t intAlign = 4;
  std::size_t longSize = 8;
  std::size_t longAlign = 8;
  std::size_t longLongSize = 8;
  std::size_t longLongAlign = 8;
  std::size_t pointerSize = 8;
  std::size_t pointerAlign = 8;
};

const TargetInfo &targetInfo();
bool isBuiltinType(const std::string &type);
std::optional<std::size_t> constSize(Expression *exp);
std::vector<std::size_t>
constDimensions(const std::vector<std::unique_ptr<Expression>> &dimensions);
std::size_t alignTo(std::size_t value, std::size_t align);
TypeInfo scalarTypeFromName(const std::string &name);
TypeLayout
layoutOf(const TypeInfo &type,
         const std::unordered_map<std::string, StructInfo> &structInfos);
std::size_t
storageBytes(const TypeInfo &type,
             const std::unordered_map<std::string, StructInfo> &structInfos);
bool isVoidObject(const TypeInfo &type);
bool isNullPointerConstant(Expression *exp);
bool isSizedType(const TypeInfo &type);
bool isLvalueExpression(Expression *exp);

class TypeResolver {
public:
  using VariableLookup =
      std::function<std::optional<TypeInfo>(const std::string &)>;
  using FunctionLookup =
      std::function<std::optional<TypeInfo>(const std::string &)>;
  using FunctionValidator =
      std::function<void(const FunctionCallExpression *)>;

  TypeResolver(VariableLookup variableLookup, FunctionLookup functionLookup,
               FunctionValidator functionValidator,
               const std::unordered_map<std::string, StructInfo> &structInfos,
               std::string currentFunction);

  TypeInfo expression(Expression *exp) const;
  TypeInfo lvalue(Expression *exp) const;

private:
  VariableLookup variableLookup;
  FunctionLookup functionLookup;
  FunctionValidator functionValidator;
  const std::unordered_map<std::string, StructInfo> &structInfos;
  std::string currentFunction;
};

#endif
