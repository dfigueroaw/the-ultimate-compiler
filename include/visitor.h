#ifndef VISITOR_H
#define VISITOR_H

#include "asm.h"
#include "ast.h"
#include "environment.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

class BinaryExpression;
class Body;
class BreakStatement;
class CompoundAssignmentStatement;
class ContinueStatement;
class DoWhileStatement;
class FunctionDeclaration;
class FunctionCallExpression;
class AddressExpression;
class DereferenceExpression;
class IdentifierExpression;
class IfStatement;
class IncDecExpression;
class NumberExpression;
class Program;
class ReturnStatement;
class Statement;
class StructDeclaration;
class SwitchStatement;
class UnaryExpression;
class VariableDeclaration;
class WhileStatement;
class SizeofExpression;
class FieldExpression;
class ForStatement;
class SubscriptExpression;
class ExpressionStatement;
class StringExpression;
class TypeResolver;

enum class TypeKind {
  Void,
  Char,
  Short,
  Int,
  Long,
  LongLong,
  Struct,
  Pointer,
  Array
};

struct TypeInfo {
  TypeKind kind = TypeKind::Int;
  bool isUnsigned = false;
  std::string name;
  std::shared_ptr<TypeInfo> element;
  std::size_t arraySize = 0;

  TypeInfo() = default;
  TypeInfo(std::string baseName, std::size_t pointerCount,
           std::vector<std::size_t> dimensions);
  static TypeInfo scalar(TypeKind kind);
  static TypeInfo structure(std::string name);
  static TypeInfo pointerTo(TypeInfo pointee);
  static TypeInfo arrayOf(std::size_t size, TypeInfo element);
  bool isPointer() const { return kind == TypeKind::Pointer; }
  bool isArray() const { return kind == TypeKind::Array; }
  bool isScalarInteger() const;
  bool isUnsignedInteger() const;
  bool isVoidObject() const;
  bool isStructObject() const;
  bool isCharObject() const;
  bool isShortObject() const;
  bool isIntObject() const;
  bool isLongObject() const;
  bool isLongLongObject() const;
  const TypeInfo &pointee() const;
  TypeInfo decayed() const;
  bool sameAfterDecay(const TypeInfo &other) const;
  bool same(const TypeInfo &other) const;
  std::vector<std::size_t> dimensions() const;
  std::string display() const;
};

struct TypeLayout {
  std::size_t size = 0;
  std::size_t align = 1;
};

struct FieldInfo {
  TypeInfo type;
  std::size_t offset = 0;
};

struct StructInfo {
  std::unordered_map<std::string, FieldInfo> fields;
  std::size_t size = 0;
  std::size_t align = 1;
};

struct SemanticInfo {
  std::unordered_map<std::string, std::size_t> frameBytes;
  std::unordered_map<std::string, StructInfo> structs;
  std::unordered_map<std::string, TypeInfo> functionReturns;
  std::unordered_map<std::string, std::vector<TypeInfo>> functionParams;
  std::unordered_map<std::string, bool> functionVariadic;
};

class Visitor {
public:
  virtual int visit(BinaryExpression *exp) = 0;
  virtual int visit(NumberExpression *exp) = 0;
  virtual int visit(StringExpression *exp) = 0;
  virtual int visit(IdentifierExpression *exp) = 0;
  virtual int visit(UnaryExpression *exp) = 0;
  virtual int visit(IncDecExpression *exp) = 0;
  virtual int visit(DereferenceExpression *exp) = 0;
  virtual int visit(AddressExpression *exp) = 0;
  virtual int visit(SubscriptExpression *exp) = 0;
  virtual int visit(SizeofExpression *exp) = 0;
  virtual int visit(FieldExpression *exp) = 0;
  virtual int visit(Program *p) = 0;
  virtual int visit(StructDeclaration *sd) = 0;
  virtual int visit(ExpressionStatement *stm) = 0;
  virtual int visit(ForStatement *stm) = 0;
  virtual int visit(WhileStatement *stm) = 0;
  virtual int visit(DoWhileStatement *stm) = 0;
  virtual int visit(IfStatement *stm) = 0;
  virtual int visit(AssignmentStatement *stm) = 0;
  virtual int visit(CompoundAssignmentStatement *stm) = 0;
  virtual int visit(BreakStatement *stm) = 0;
  virtual int visit(ContinueStatement *stm) = 0;
  virtual int visit(SwitchStatement *stm) = 0;
  virtual int visit(Body *body) = 0;
  virtual int visit(VariableDeclaration *vd) = 0;
  virtual int visit(FunctionCallExpression *fc) = 0;
  virtual int visit(ReturnStatement *r) = 0;
  virtual int visit(FunctionDeclaration *fd) = 0;
};

class TypeCheckerVisitor : public Visitor {
public:
  std::unordered_map<std::string, std::size_t> functionFrameBytes;

  std::unordered_map<std::string, StructInfo> structInfos;

  std::size_t localBytes = 0;

  Environment<int> symbolScopes;
  Environment<TypeInfo> variableTypes;
  std::unordered_map<std::string, TypeInfo> functionReturnTypes;
  std::unordered_map<std::string, std::vector<TypeInfo>> functionParameterTypes;
  std::unordered_map<std::string, bool> functionVariadicFlags;
  std::unordered_map<std::string, bool> functionDefinedFlags;

  std::string currentFunction;
  TypeInfo currentReturnType;
  std::size_t breakableDepth = 0;
  std::size_t loopDepth = 0;

  SemanticInfo analyze(Program *program);
  bool isKnownType(const std::string &type) const;
  bool sameSignature(const FunctionDeclaration *function,
                     const TypeInfo &returnType,
                     const std::vector<TypeInfo> &params) const;
  void requireKnownType(const std::string &type,
                        const std::string &context) const;
  TypeInfo typeFromDeclarator(const std::string &base,
                              const Declarator &declarator) const;
  TypeInfo infer(Expression *exp) const;
  TypeInfo inferLvalue(Expression *exp) const;
  void requireAssignable(const TypeInfo &target, const TypeInfo &source,
                         const std::string &context) const;
  void requireAssignableExpression(const TypeInfo &target, Expression *source,
                                   const std::string &context) const;
  void requireUserFunctionCall(const FunctionCallExpression *call) const;
  void requireCondition(Expression *exp, const std::string &context) const;
  void requireInteger(Expression *exp, const std::string &context) const;
  bool statementAlwaysReturns(const Statement *stm) const;
  bool statementListAlwaysReturns(
      const std::list<std::unique_ptr<Statement>> &statements) const;
  bool bodyAlwaysReturns(const Body *body) const;
  bool switchAlwaysReturns(const SwitchStatement *stm) const;

  int visit(BinaryExpression *exp) override;
  int visit(NumberExpression *exp) override;
  int visit(StringExpression *exp) override;
  int visit(IdentifierExpression *exp) override;
  int visit(UnaryExpression *exp) override;
  int visit(IncDecExpression *exp) override;
  int visit(DereferenceExpression *exp) override;
  int visit(AddressExpression *exp) override;
  int visit(SubscriptExpression *exp) override;
  int visit(SizeofExpression *exp) override;
  int visit(FieldExpression *exp) override;
  int visit(Program *p) override;
  int visit(StructDeclaration *sd) override;
  int visit(ExpressionStatement *stm) override;
  int visit(ForStatement *stm) override;
  int visit(AssignmentStatement *stm) override;
  int visit(CompoundAssignmentStatement *stm) override;
  int visit(WhileStatement *stm) override;
  int visit(DoWhileStatement *stm) override;
  int visit(IfStatement *stm) override;
  int visit(BreakStatement *stm) override;
  int visit(ContinueStatement *stm) override;
  int visit(SwitchStatement *stm) override;
  int visit(Body *body) override;
  int visit(VariableDeclaration *vd) override;
  int visit(FunctionCallExpression *fc) override;
  int visit(ReturnStatement *r) override;
  int visit(FunctionDeclaration *fd) override;
};

enum class LValueKind { Invalid, Id, Subscript, Deref, Field };

struct LValue {
  LValueKind kind = LValueKind::Invalid;
  std::string name;
  Expression *index = nullptr;
  Expression *expression = nullptr;
  std::string field;
};

class GenCodeVisitor : public Visitor {
private:
  std::ostream &out;
  AsmBuilder assembly;
  LValue *lvalueTarget = nullptr;
  Expression *hiddenStructReturnTarget = nullptr;
  std::unordered_map<const StringExpression *, std::string> stringLabels;
  std::vector<const StringExpression *> stringLiteralOrder;
  LValue captureLValue(Expression *exp);
  void collectStringLiterals(Program *program);
  void collectStringLiterals(Body *body);
  void collectStringLiterals(VariableDeclaration *declaration);
  void collectStringLiterals(Statement *statement);
  void collectStringLiterals(Expression *expression);
  std::string labelForString(const StringExpression *exp);
  void emitStringLiterals();
  TypeInfo variableInfo(const std::string &name) const;
  TypeResolver typeResolver() const;
  TypeInfo lvalueInfo(Expression *exp) const;
  TypeInfo expressionInfo(Expression *exp) const;
  AsmWidth storageWidth(const TypeInfo &type) const;
  AsmWidth expressionWidth(const TypeInfo &type) const;
  void normalizeReturnValue(const TypeInfo &type);
  void normalizeScalarValue(const TypeInfo &type);
  bool returnsViaHiddenPointer(const TypeInfo &type) const;
  AsmOperand variableMemory(const std::string &name) const;
  void emitLoadAddressed(const TypeInfo &type, AsmOperand address,
                         const std::string &reg = "%rax");
  void emitStoreAddressed(const TypeInfo &type, AsmOperand address,
                          AsmRegister reg = AsmRegister::RAX);
  void emitLoadVariable(const std::string &name,
                        const std::string &reg = "%rax");
  void emitStoreVariable(const std::string &name,
                         AsmRegister reg = AsmRegister::RAX);
  void emitVariableAddress(const std::string &name,
                           const std::string &reg = "%rax");
  void emitExpressionAddress(Expression *exp, const std::string &reg = "%rax");
  void emitSubscriptAddress(const LValue &lv);
  void emitCopyBytes(std::size_t bytes);
  void emitLoadAggregateSlot(Expression *source, std::size_t offset,
                             std::size_t bytes, AsmRegister reg);
  void emitStoreAggregateSlot(Expression *target, std::size_t offset,
                              std::size_t bytes, AsmRegister reg);
  void emitStoreAggregateSlotToAddress(std::size_t offset, std::size_t bytes,
                                       AsmRegister reg);
  void emitStoreAggregateReturn(Expression *target, const TypeInfo &type);
  void emitUserFunctionCall(FunctionCallExpression *exp,
                            bool hiddenStructReturn);
  void emitPush(AsmOperand operand);
  void emitPop(AsmOperand operand);
  void emitCall(const std::string &target);
  void emitMoveArgumentToRegister(const TypeInfo &type, AsmRegister reg);
  int storeTarget(const LValue &lv);
  int storeId(const LValue &lv);
  int storeSubscript(const LValue &lv);
  int storeDeref(const LValue &lv);
  int storeField(const LValue &lv);

public:
  TypeCheckerVisitor typeChecker;
  std::unordered_map<std::string, std::size_t> functionFrameBytes;
  std::unordered_map<std::string, StructInfo> structInfos;
  std::unordered_map<std::string, TypeInfo> functionReturnTypes;
  std::unordered_map<std::string, std::vector<TypeInfo>> functionParameterTypes;
  std::unordered_map<std::string, bool> functionVariadicFlags;
  std::unordered_map<std::string, int> localOffsets;
  std::unordered_map<std::string, TypeInfo> variableInfos;
  std::unordered_map<std::string, TypeInfo> globalVariableInfos;
  std::unordered_map<std::string, std::int64_t> globalInitializers;
  std::unordered_map<std::string, const StringExpression *>
      globalStringInitializers;
  std::unordered_map<std::string, bool> globalVariables;
  int offset = -8;
  std::size_t evalStackBytes = 0;
  std::size_t labelCounter = 0;
  bool insideFunction = false;
  int hiddenReturnOffset = 0;
  std::string currentBreakLabel;
  std::string currentContinueLabel;
  std::string currentFunctionName;

  explicit GenCodeVisitor(std::ostream &out) : out(out) {}

  int generate(Program *program);
  std::size_t typeSize(const TypeInfo &type) const;
  std::size_t expressionSize(Expression *exp) const;

  int visit(BinaryExpression *exp) override;
  int visit(NumberExpression *exp) override;
  int visit(StringExpression *exp) override;
  int visit(IdentifierExpression *exp) override;
  int visit(UnaryExpression *exp) override;
  int visit(IncDecExpression *exp) override;
  int visit(DereferenceExpression *exp) override;
  int visit(AddressExpression *exp) override;
  int visit(SubscriptExpression *exp) override;
  int visit(SizeofExpression *exp) override;
  int visit(FieldExpression *exp) override;
  int visit(Program *p) override;
  int visit(StructDeclaration *sd) override;
  int visit(ExpressionStatement *stm) override;
  int visit(ForStatement *stm) override;
  int visit(AssignmentStatement *stm) override;
  int visit(CompoundAssignmentStatement *stm) override;
  int visit(WhileStatement *stm) override;
  int visit(DoWhileStatement *stm) override;
  int visit(IfStatement *stm) override;
  int visit(BreakStatement *stm) override;
  int visit(ContinueStatement *stm) override;
  int visit(SwitchStatement *stm) override;
  int visit(Body *body) override;
  int visit(VariableDeclaration *vd) override;
  int visit(FunctionCallExpression *fc) override;
  int visit(ReturnStatement *r) override;
  int visit(FunctionDeclaration *fd) override;
};

#endif
