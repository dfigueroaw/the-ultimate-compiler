#include "../include/constant_folder.h"
#include "../include/visitor_utils.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace {
constexpr std::array argRegisters{AsmRegister::RDI, AsmRegister::RSI,
                                  AsmRegister::RDX, AsmRegister::RCX,
                                  AsmRegister::R8,  AsmRegister::R9};
constexpr std::array returnRegisters{AsmRegister::RAX, AsmRegister::RDX};

struct AbiSlot {
  std::size_t offset = 0;
  std::size_t bytes = 0;
};

struct AbiInfo {
  bool memory = false;
  std::size_t size = 0;
  std::size_t align = 1;
  std::vector<AbiSlot> slots;
};

AsmOperand asmReg(AsmRegister r, AsmWidth width = AsmWidth::Quad) {
  return AsmOperand::regOp(r, width);
}

AsmOperand regText(const std::string &r) { return AsmOperand::regText(r); }

AsmOperand imm(long value) { return AsmOperand::imm(value); }

AsmOperand stackMem(long displacement) {
  return AsmOperand::stack(displacement);
}

AsmOperand globalMem(const std::string &name) {
  return AsmOperand::global(name);
}

AsmOperand indirectMem(AsmRegister base = AsmRegister::RAX,
                       long displacement = 0) {
  return AsmOperand::indirect(base, displacement);
}

AbiInfo classifyAbi(
    const TypeInfo &type,
    const std::unordered_map<std::string, StructInfo> &structInfos) {
  const TypeInfo decayed = type.decayed();
  const TypeLayout layout = layoutOf(decayed, structInfos);
  AbiInfo info;
  info.size = layout.size;
  info.align = layout.align;

  if (!decayed.isStructObject()) {
    info.slots.push_back(AbiSlot{0, std::min<std::size_t>(layout.size, 8)});
    return info;
  }

  if (layout.size > 16) {
    info.memory = true;
    return info;
  }

  for (std::size_t offset = 0; offset < layout.size; offset += 8)
    info.slots.push_back(
        AbiSlot{offset, std::min<std::size_t>(8, layout.size - offset)});
  return info;
}

} // namespace

LValue GenCodeVisitor::captureLValue(Expression *exp) {
  LValue lv;
  lvalueTarget = &lv;
  exp->accept(this);
  lvalueTarget = nullptr;
  return lv;
}

int GenCodeVisitor::storeTarget(const LValue &lv) {
  switch (lv.kind) {
  case LValueKind::Id:
    return storeId(lv);
  case LValueKind::Subscript:
    return storeSubscript(lv);
  case LValueKind::Deref:
    return storeDeref(lv);
  case LValueKind::Field:
    return storeField(lv);
  default:
    throw std::runtime_error("Asignacion a una expression que no es lvalue");
  }
}

int GenCodeVisitor::generate(Program *program) {
  const SemanticInfo semantics = typeChecker.analyze(program);
  foldConstants(program, semantics);
  functionFrameBytes = semantics.frameBytes;
  structInfos = semantics.structs;
  functionReturnTypes = semantics.functionReturns;
  functionParameterTypes = semantics.functionParams;
  functionVariadicFlags = semantics.functionVariadic;
  collectStringLiterals(program);
  program->accept(this);
  assembly.write(out);
  return 0;
}

void GenCodeVisitor::collectStringLiterals(Program *program) {
  for (auto &declaration : program->globalDeclarations)
    collectStringLiterals(declaration.get());
  for (auto &function : program->functionDeclarations)
    if (function->hasBody())
      collectStringLiterals(function->body.get());
}

void GenCodeVisitor::collectStringLiterals(Body *body) {
  for (auto &item : body->items) {
    if (auto *declaration = dynamic_cast<DeclarationItem *>(item.get())) {
      collectStringLiterals(declaration->declaration.get());
    } else if (auto *statement = dynamic_cast<StatementItem *>(item.get())) {
      collectStringLiterals(statement->statement.get());
    }
  }
}

void GenCodeVisitor::collectStringLiterals(VariableDeclaration *declaration) {
  for (auto &declarator : declaration->declarators) {
    for (auto &dimension : declarator.dimensions)
      collectStringLiterals(dimension.get());
    if (declarator.initializer)
      collectStringLiterals(declarator.initializer.get());
  }
}

void GenCodeVisitor::collectStringLiterals(Statement *statement) {
  if (auto *assign = dynamic_cast<AssignmentStatement *>(statement)) {
    collectStringLiterals(assign->target.get());
    collectStringLiterals(assign->expression.get());
  } else if (auto *assign =
                 dynamic_cast<CompoundAssignmentStatement *>(statement)) {
    collectStringLiterals(assign->target.get());
    collectStringLiterals(assign->expression.get());
  } else if (auto *expr = dynamic_cast<ExpressionStatement *>(statement)) {
    collectStringLiterals(expr->expression.get());
  } else if (auto *ret = dynamic_cast<ReturnStatement *>(statement)) {
    if (ret->expression)
      collectStringLiterals(ret->expression.get());
  } else if (auto *ifStm = dynamic_cast<IfStatement *>(statement)) {
    collectStringLiterals(ifStm->condition.get());
    collectStringLiterals(ifStm->then.get());
    if (ifStm->elseBody)
      collectStringLiterals(ifStm->elseBody.get());
  } else if (auto *forStm = dynamic_cast<ForStatement *>(statement)) {
    std::visit(
        [&](auto &initializer) {
          using Initializer = std::decay_t<decltype(initializer)>;
          if constexpr (!std::is_same_v<Initializer, std::monostate>) {
            if (initializer)
              collectStringLiterals(initializer.get());
          }
        },
        forStm->initializer);
    collectStringLiterals(forStm->condition.get());
    if (forStm->update)
      collectStringLiterals(forStm->update.get());
    collectStringLiterals(forStm->body.get());
  } else if (auto *whileStm = dynamic_cast<WhileStatement *>(statement)) {
    collectStringLiterals(whileStm->condition.get());
    collectStringLiterals(whileStm->body.get());
  } else if (auto *doWhile = dynamic_cast<DoWhileStatement *>(statement)) {
    collectStringLiterals(doWhile->body.get());
    collectStringLiterals(doWhile->condition.get());
  } else if (auto *switchStm = dynamic_cast<SwitchStatement *>(statement)) {
    collectStringLiterals(switchStm->expression.get());
    for (auto &caseStm : switchStm->cases)
      for (auto &caseBodyStatement : caseStm->body)
        collectStringLiterals(caseBodyStatement.get());
    for (auto &defaultStatement : switchStm->defaultBody)
      collectStringLiterals(defaultStatement.get());
  }
}

void GenCodeVisitor::collectStringLiterals(Expression *expression) {
  if (!expression)
    return;
  if (auto *string = dynamic_cast<StringExpression *>(expression)) {
    labelForString(string);
  } else if (auto *binary = dynamic_cast<BinaryExpression *>(expression)) {
    collectStringLiterals(binary->left.get());
    collectStringLiterals(binary->right.get());
  } else if (auto *unary = dynamic_cast<UnaryExpression *>(expression)) {
    collectStringLiterals(unary->operand.get());
  } else if (auto *incDec = dynamic_cast<IncDecExpression *>(expression)) {
    collectStringLiterals(incDec->operand.get());
  } else if (auto *subscript =
                 dynamic_cast<SubscriptExpression *>(expression)) {
    collectStringLiterals(subscript->base.get());
    for (auto &index : subscript->indices)
      collectStringLiterals(index.get());
  } else if (auto *deref = dynamic_cast<DereferenceExpression *>(expression)) {
    collectStringLiterals(deref->operand.get());
  } else if (auto *address = dynamic_cast<AddressExpression *>(expression)) {
    collectStringLiterals(address->operand.get());
  } else if (auto *size = dynamic_cast<SizeofExpression *>(expression)) {
    collectStringLiterals(size->expression.get());
  } else if (auto *call = dynamic_cast<FunctionCallExpression *>(expression)) {
    for (auto &argument : call->arguments)
      collectStringLiterals(argument.get());
  } else if (auto *field = dynamic_cast<FieldExpression *>(expression)) {
    collectStringLiterals(field->object.get());
  }
}

std::string GenCodeVisitor::labelForString(const StringExpression *exp) {
  if (const auto found = stringLabels.find(exp); found != stringLabels.end())
    return found->second;

  const std::string label = ".LC" + std::to_string(stringLabels.size());
  stringLabels[exp] = label;
  stringLiteralOrder.push_back(exp);
  return label;
}

void GenCodeVisitor::emitStringLiterals() {
  for (const auto *literal : stringLiteralOrder) {
    assembly.directive(labelForString(literal) + ":");
    std::string bytes = ".byte ";
    for (unsigned char c : literal->value) {
      if (bytes != ".byte ")
        bytes += ", ";
      bytes += std::to_string(static_cast<unsigned int>(c));
    }
    if (bytes != ".byte ")
      bytes += ", ";
    bytes += "0";
    assembly.directive(bytes);
  }
}

std::size_t GenCodeVisitor::typeSize(const TypeInfo &type) const {
  return storageBytes(type, structInfos);
}

std::size_t GenCodeVisitor::expressionSize(Expression *exp) const {
  if (auto *string = dynamic_cast<StringExpression *>(exp))
    return typeSize(TypeInfo{"char", 0, {string->value.size() + 1}});
  const TypeInfo type =
      isLvalueExpression(exp) ? lvalueInfo(exp) : expressionInfo(exp).decayed();
  return typeSize(type);
}

TypeInfo GenCodeVisitor::variableInfo(const std::string &name) const {
  return variableInfos.at(name);
}

TypeResolver GenCodeVisitor::typeResolver() const {
  return TypeResolver(
      [&](const std::string &name) -> std::optional<TypeInfo> {
        if (const auto found = variableInfos.find(name);
            found != variableInfos.end())
          return found->second;
        return std::nullopt;
      },
      [&](const std::string &name) -> std::optional<TypeInfo> {
        if (const auto found = functionReturnTypes.find(name);
            found != functionReturnTypes.end())
          return found->second;
        return std::nullopt;
      },
      nullptr, structInfos, currentFunctionName);
}

TypeInfo GenCodeVisitor::lvalueInfo(Expression *exp) const {
  auto resolver = typeResolver();
  return resolver.lvalue(exp);
}

TypeInfo GenCodeVisitor::expressionInfo(Expression *exp) const {
  auto resolver = typeResolver();
  return resolver.expression(exp);
}

AsmWidth GenCodeVisitor::storageWidth(const TypeInfo &type) const {
  const TypeInfo decayed = type.decayed();
  if (isPointerType(decayed))
    return AsmWidth::Quad;
  const auto bytes = typeSize(decayed);
  if (bytes == 1)
    return AsmWidth::Byte;
  if (bytes == 2)
    return AsmWidth::Word;
  if (bytes == 4)
    return AsmWidth::Long;
  if (bytes == 8)
    return AsmWidth::Quad;
  throw std::runtime_error("[CodeGen] Ancho escalar no soportado");
}

AsmWidth GenCodeVisitor::expressionWidth(const TypeInfo &type) const {
  const TypeInfo decayed = type.decayed();
  if (isPointerType(decayed))
    return AsmWidth::Quad;
  if (isScalarInteger(decayed))
    return storageWidth(integerPromotion(decayed));
  return storageWidth(decayed);
}

void GenCodeVisitor::normalizeReturnValue(const TypeInfo &type) {
  normalizeScalarValue(type);
}

void GenCodeVisitor::normalizeScalarValue(const TypeInfo &type) {
  const TypeInfo decayed = type.decayed();
  if (isPointerType(decayed))
    return;
  if (!isScalarInteger(decayed))
    return;
  const AsmWidth width = storageWidth(decayed);
  if (width != AsmWidth::Quad)
    assembly.movsx(width, AsmWidth::Quad, asmReg(AsmRegister::RAX, width),
                   asmReg(AsmRegister::RAX));
}

bool GenCodeVisitor::returnsViaHiddenPointer(const TypeInfo &type) const {
  const AbiInfo abi = classifyAbi(type, structInfos);
  return abi.memory;
}

AsmOperand GenCodeVisitor::variableMemory(const std::string &name) const {
  if (localOffsets.count(name))
    return AsmOperand::stack(localOffsets.at(name));
  return AsmOperand::global(name);
}

void GenCodeVisitor::emitLoadAddressed(const TypeInfo &type, AsmOperand address,
                                       const std::string &reg) {
  const auto decayed = type.decayed();
  if (isPointerType(decayed)) {
    assembly.mov(AsmWidth::Quad, std::move(address), regText(reg));
  } else if (isScalarInteger(decayed)) {
    const AsmWidth width = storageWidth(decayed);
    if (width == AsmWidth::Quad)
      assembly.mov(AsmWidth::Quad, std::move(address), regText(reg));
    else
      assembly.movsx(width, AsmWidth::Quad, std::move(address), regText(reg));
  } else {
    throw std::runtime_error("[CodeGen] Carga escalar no soportada");
  }
}

void GenCodeVisitor::emitStoreAddressed(const TypeInfo &type,
                                        AsmOperand address, AsmRegister reg) {
  const auto decayed = type.decayed();
  if (isPointerType(decayed)) {
    assembly.mov(AsmWidth::Quad, asmReg(reg), std::move(address));
  } else if (isScalarInteger(decayed)) {
    const AsmWidth width = storageWidth(decayed);
    assembly.mov(width, asmReg(reg, width), std::move(address));
  } else {
    throw std::runtime_error("[CodeGen] Almacenamiento escalar no soportado");
  }
}

void GenCodeVisitor::emitLoadVariable(const std::string &name,
                                      const std::string &reg) {
  emitLoadAddressed(variableInfo(name), variableMemory(name), reg);
}

void GenCodeVisitor::emitStoreVariable(const std::string &name,
                                       AsmRegister reg) {
  emitStoreAddressed(variableInfo(name), variableMemory(name), reg);
}

void GenCodeVisitor::emitVariableAddress(const std::string &name,
                                         const std::string &reg) {
  if (localOffsets.count(name))
    assembly.lea(stackMem(localOffsets[name]), regText(reg));
  else
    assembly.lea(globalMem(name), regText(reg));
}

void GenCodeVisitor::emitExpressionAddress(Expression *exp,
                                           const std::string &reg) {
  if (auto *id = dynamic_cast<IdentifierExpression *>(exp)) {
    emitVariableAddress(id->value, reg);
    return;
  }
  if (auto *subscript = dynamic_cast<SubscriptExpression *>(exp)) {
    LValue lv;
    lv.kind = LValueKind::Subscript;
    lv.expression = subscript;
    for (auto &index : subscript->indices)
      lv.indices.push_back(index.get());
    emitSubscriptAddress(lv);
    if (reg != "%rax")
      assembly.mov(AsmWidth::Quad, asmReg(AsmRegister::RAX), regText(reg));
    return;
  }
  if (auto *deref = dynamic_cast<DereferenceExpression *>(exp)) {
    deref->operand->accept(this);
    if (reg != "%rax")
      assembly.mov(AsmWidth::Quad, asmReg(AsmRegister::RAX), regText(reg));
    return;
  }
  if (auto *field = dynamic_cast<FieldExpression *>(exp)) {
    TypeInfo base = field->throughPointer
                        ? expressionInfo(field->object.get()).decayed()
                        : expressionInfo(field->object.get());
    if (field->throughPointer) {
      field->object->accept(this);
      if (isPointerType(base))
        base = pointedType(base);
    } else {
      emitExpressionAddress(field->object.get());
    }
    const auto offset =
        structInfos.at(base.name).fields.at(field->field).offset;
    if (offset != 0)
      assembly.add(AsmWidth::Quad, imm(static_cast<long>(offset)),
                   asmReg(AsmRegister::RAX));
    if (reg != "%rax")
      assembly.mov(AsmWidth::Quad, asmReg(AsmRegister::RAX), regText(reg));
    return;
  }
  throw std::runtime_error("No se puede obtener dirección de esta expresión");
}

void GenCodeVisitor::emitSubscriptAddress(const LValue &lv) {
  auto *subscript = dynamic_cast<SubscriptExpression *>(lv.expression);
  if (!subscript)
    throw std::runtime_error("[CodeGen] Subscript inválido");

  TypeInfo baseType = lvalueInfo(subscript->base.get());
  const std::vector<std::size_t> dimensions = baseType.dimensions();

  assembly.mov(AsmWidth::Quad, imm(0), asmReg(AsmRegister::R11));
  for (std::size_t i = 0; i < lv.indices.size(); ++i) {
    if (i > 0) {
      const std::size_t dimension = i < dimensions.size() ? dimensions[i] : 1;
      assembly.imul(AsmWidth::Quad, imm(static_cast<long>(dimension)),
                    asmReg(AsmRegister::R11));
    }
    emitPush(asmReg(AsmRegister::R11));
    lv.indices[i]->accept(this);
    emitPop(asmReg(AsmRegister::R11));
    assembly.add(AsmWidth::Quad, asmReg(AsmRegister::RAX),
                 asmReg(AsmRegister::R11));
  }
  TypeInfo elementType = baseType;
  if (!lv.indices.empty())
    elementType = lvalueInfo(lv.expression);
  const std::size_t elementSize =
      std::max<std::size_t>(typeSize(elementType), 1);
  if (elementSize != 1)
    assembly.imul(AsmWidth::Quad, imm(static_cast<long>(elementSize)),
                  asmReg(AsmRegister::R11));

  emitPush(asmReg(AsmRegister::R11));
  if (baseType.isArray()) {
    emitExpressionAddress(subscript->base.get());
  } else {
    subscript->base->accept(this);
  }
  emitPop(asmReg(AsmRegister::R11));
  assembly.add(AsmWidth::Quad, asmReg(AsmRegister::R11),
               asmReg(AsmRegister::RAX));
}

void GenCodeVisitor::emitCopyBytes(std::size_t bytes) {
  std::size_t copied = 0;
  while (copied + 8 <= bytes) {
    assembly.mov(
        AsmWidth::Quad,
        AsmOperand::indirect(AsmRegister::RSI, static_cast<long>(copied)),
        asmReg(AsmRegister::RAX));
    assembly.mov(
        AsmWidth::Quad, asmReg(AsmRegister::RAX),
        AsmOperand::indirect(AsmRegister::RDI, static_cast<long>(copied)));
    copied += 8;
  }
  while (copied + 4 <= bytes) {
    assembly.mov(
        AsmWidth::Long,
        AsmOperand::indirect(AsmRegister::RSI, static_cast<long>(copied)),
        asmReg(AsmRegister::RAX, AsmWidth::Long));
    assembly.mov(
        AsmWidth::Long, asmReg(AsmRegister::RAX, AsmWidth::Long),
        AsmOperand::indirect(AsmRegister::RDI, static_cast<long>(copied)));
    copied += 4;
  }
  while (copied < bytes) {
    assembly.mov(
        AsmWidth::Byte,
        AsmOperand::indirect(AsmRegister::RSI, static_cast<long>(copied)),
        asmReg(AsmRegister::RAX, AsmWidth::Byte));
    assembly.mov(
        AsmWidth::Byte, asmReg(AsmRegister::RAX, AsmWidth::Byte),
        AsmOperand::indirect(AsmRegister::RDI, static_cast<long>(copied)));
    copied += 1;
  }
}

void GenCodeVisitor::emitLoadAggregateSlot(Expression *source,
                                           std::size_t offset,
                                           std::size_t bytes,
                                           AsmRegister reg) {
  assembly.sub(AsmWidth::Quad, imm(8), asmReg(AsmRegister::RSP));
  evalStackBytes += 8;
  assembly.mov(AsmWidth::Quad, imm(0), indirectMem(AsmRegister::RSP));

  emitExpressionAddress(source, "%rsi");
  if (offset != 0)
    assembly.lea(indirectMem(AsmRegister::RSI, static_cast<long>(offset)),
                 asmReg(AsmRegister::RSI));
  assembly.lea(indirectMem(AsmRegister::RSP), asmReg(AsmRegister::RDI));
  emitCopyBytes(bytes);
  emitPop(asmReg(reg));
}

void GenCodeVisitor::emitStoreAggregateSlot(Expression *target,
                                            std::size_t offset,
                                            std::size_t bytes,
                                            AsmRegister reg) {
  emitExpressionAddress(target, "%rdi");
  emitStoreAggregateSlotToAddress(offset, bytes, reg);
}

void GenCodeVisitor::emitStoreAggregateSlotToAddress(std::size_t offset,
                                                     std::size_t bytes,
                                                     AsmRegister reg) {
  if (offset != 0)
    assembly.lea(indirectMem(AsmRegister::RDI, static_cast<long>(offset)),
                 asmReg(AsmRegister::RDI));
  emitPush(asmReg(reg));
  assembly.lea(indirectMem(AsmRegister::RSP), asmReg(AsmRegister::RSI));
  emitCopyBytes(bytes);
  assembly.add(AsmWidth::Quad, imm(8), asmReg(AsmRegister::RSP));
  if (evalStackBytes < 8)
    throw std::runtime_error("[CodeGen] Desbalance de pila temporal");
  evalStackBytes -= 8;
}

void GenCodeVisitor::emitStoreAggregateReturn(Expression *target,
                                              const TypeInfo &type) {
  const AbiInfo abi = classifyAbi(type, structInfos);
  if (abi.memory)
    throw std::runtime_error(
        "[CodeGen] Retorno agregado en memoria requiere destino oculto");
  for (std::size_t i = 0; i < abi.slots.size(); ++i)
    emitStoreAggregateSlot(target, abi.slots[i].offset, abi.slots[i].bytes,
                           returnRegisters[i]);
}

void GenCodeVisitor::emitPush(AsmOperand operand) {
  assembly.push(std::move(operand));
  evalStackBytes += 8;
}

void GenCodeVisitor::emitPop(AsmOperand operand) {
  assembly.pop(std::move(operand));
  if (evalStackBytes < 8)
    throw std::runtime_error("[CodeGen] Desbalance de pila temporal");
  evalStackBytes -= 8;
}

void GenCodeVisitor::emitCall(const std::string &target) {
  const bool needsAlignment = evalStackBytes % 16 != 0;
  if (needsAlignment)
    assembly.sub(AsmWidth::Quad, imm(8), asmReg(AsmRegister::RSP));
  assembly.call(target);
  if (needsAlignment)
    assembly.add(AsmWidth::Quad, imm(8), asmReg(AsmRegister::RSP));
}

void GenCodeVisitor::emitMoveArgumentToRegister(const TypeInfo &type,
                                                AsmRegister reg) {
  const TypeInfo decayed = type.decayed();
  if (isPointerType(decayed)) {
    assembly.mov(AsmWidth::Quad, asmReg(AsmRegister::RAX), asmReg(reg));
  } else if (isScalarInteger(decayed)) {
    const AsmWidth width = storageWidth(decayed);
    assembly.mov(width, asmReg(AsmRegister::RAX, width), asmReg(reg, width));
  } else {
    throw std::runtime_error("[CodeGen] Argumento escalar no soportado");
  }
}

int GenCodeVisitor::visit(Program *program) {
  assembly.directive(".data");

  for (auto &dec : program->globalDeclarations)
    dec->accept(this);

  for (const auto &entry : globalVariables) {
    const TypeInfo globalType = variableInfo(entry.first);
    if (globalType.isArray())
      assembly.directive(entry.first + ": .zero " +
                         std::to_string(typeSize(globalType)));
    else if (globalType.kind == TypeKind::Struct)
      assembly.directive(entry.first + ": .zero " +
                         std::to_string(typeSize(globalType)));
    else if (globalInitializers.contains(entry.first)) {
      if (isPointerType(globalType))
        assembly.directive(entry.first + ": .quad " +
                           std::to_string(globalInitializers[entry.first]));
      else {
        const auto width = storageWidth(globalType);
        std::string directive = ".long ";
        if (width == AsmWidth::Byte)
          directive = ".byte ";
        else if (width == AsmWidth::Word)
          directive = ".short ";
        else if (width == AsmWidth::Quad)
          directive = ".quad ";
        assembly.directive(entry.first + ": " + directive +
                           std::to_string(globalInitializers[entry.first]));
      }
    } else if (globalStringInitializers.contains(entry.first)) {
      assembly.directive(entry.first + ": .quad " +
                         labelForString(globalStringInitializers[entry.first]));
    } else
      assembly.directive(entry.first + ": .zero " +
                         std::to_string(typeSize(globalType)));
  }
  emitStringLiterals();
  assembly.blank();
  assembly.directive(".text");
  for (auto &fd : program->functionDeclarations)
    if (fd->hasBody())
      fd->accept(this);

  assembly.blank();
  assembly.directive(".section .note.GNU-stack,\"\",@progbits");
  return 0;
}

int GenCodeVisitor::visit(StructDeclaration *) { return 0; }

int GenCodeVisitor::visit(VariableDeclaration *stm) {
  for (auto &declarator : stm->declarators) {
    const auto &var = declarator.name;
    auto dimensions = constDimensions(declarator.dimensions);
    const TypeInfo type{stm->baseType, declarator.pointerDepth, dimensions};
    const auto layout = layoutOf(type, structInfos);
    const std::size_t bytes = alignTo(layout.size, layout.align);
    if (!insideFunction) {
      globalVariables[var] = true;
      variableInfos[var] = type;
      globalVariableInfos[var] = type;
      if (auto *number =
              dynamic_cast<NumberExpression *>(declarator.initializer.get()))
        globalInitializers[var] = number->value;
      if (auto *string =
              dynamic_cast<StringExpression *>(declarator.initializer.get()))
        globalStringInitializers[var] = string;
    } else {
      offset -= static_cast<int>(bytes);
      const int remainder = (-offset) % static_cast<int>(layout.align);
      if (remainder != 0)
        offset -= static_cast<int>(layout.align) - remainder;
      localOffsets[var] = offset;
      variableInfos[var] = type;
    }
    if (insideFunction && declarator.initializer) {
      declarator.initializer->accept(this);
      emitStoreVariable(var);
    }
  }
  return 0;
}

int GenCodeVisitor::visit(NumberExpression *exp) {
  assembly.mov(AsmWidth::Quad, imm(exp->value), asmReg(AsmRegister::RAX));
  return 0;
}

int GenCodeVisitor::visit(StringExpression *exp) {
  assembly.lea(globalMem(labelForString(exp)), asmReg(AsmRegister::RAX));
  return 0;
}

int GenCodeVisitor::visit(IdentifierExpression *exp) {
  if (lvalueTarget) {
    lvalueTarget->kind = LValueKind::Id;
    lvalueTarget->name = exp->value;
    return 0;
  }
  if (variableInfo(exp->value).isArray()) {
    if (globalVariables.count(exp->value))
      assembly.lea(globalMem(exp->value), asmReg(AsmRegister::RAX));
    else
      assembly.lea(stackMem(localOffsets[exp->value]),
                   asmReg(AsmRegister::RAX));
    return 0;
  }
  emitLoadVariable(exp->value);
  return 0;
}

int GenCodeVisitor::visit(SubscriptExpression *exp) {
  if (lvalueTarget) {
    lvalueTarget->kind = LValueKind::Subscript;
    lvalueTarget->expression = exp;
    lvalueTarget->indices.clear();
    for (auto &index : exp->indices)
      lvalueTarget->indices.push_back(index.get());
    return 0;
  }
  LValue lv;
  lv.kind = LValueKind::Subscript;
  lv.expression = exp;
  for (auto &index : exp->indices)
    lv.indices.push_back(index.get());
  emitSubscriptAddress(lv);
  const TypeInfo target = lvalueInfo(exp);
  if (target.isArray())
    return 0;
  emitLoadAddressed(expressionInfo(exp), indirectMem());
  return 0;
}

int GenCodeVisitor::visit(DereferenceExpression *exp) {
  if (lvalueTarget) {
    lvalueTarget->kind = LValueKind::Deref;
    lvalueTarget->expression = exp->operand.get();
    return 0;
  }
  exp->operand->accept(this);
  emitLoadAddressed(expressionInfo(exp), indirectMem());
  return 0;
}

int GenCodeVisitor::visit(AddressExpression *exp) {
  emitExpressionAddress(exp->operand.get());
  return 0;
}

int GenCodeVisitor::visit(FieldExpression *exp) {
  if (lvalueTarget) {
    lvalueTarget->kind = LValueKind::Field;
    lvalueTarget->expression = exp;
    lvalueTarget->field = exp->field;
    return 0;
  }
  emitExpressionAddress(exp);
  if (lvalueInfo(exp).isArray())
    return 0;
  emitLoadAddressed(expressionInfo(exp), indirectMem());
  return 0;
}

int GenCodeVisitor::visit(SizeofExpression *exp) {
  std::size_t size = 8;
  if (exp->expression) {
    size = expressionSize(exp->expression.get());
  } else {
    size = typeSize(TypeInfo{exp->baseType, exp->pointerDepth, {}});
  }
  assembly.mov(AsmWidth::Quad, imm(static_cast<long>(size)),
               asmReg(AsmRegister::RAX));
  return 0;
}

int GenCodeVisitor::visit(AssignmentStatement *stm) {
  LValue target = captureLValue(stm->target.get());
  const TypeInfo targetType = lvalueInfo(stm->target.get());
  if (targetType.isStructObject()) {
    if (auto *call =
            dynamic_cast<FunctionCallExpression *>(stm->expression.get())) {
      const TypeInfo returnType = functionReturnTypes.at(call->name);
      if (returnsViaHiddenPointer(returnType)) {
        hiddenStructReturnTarget = stm->target.get();
        emitUserFunctionCall(call, true);
        hiddenStructReturnTarget = nullptr;
      } else {
        emitUserFunctionCall(call, false);
        emitStoreAggregateReturn(stm->target.get(), returnType);
      }
      return 0;
    }
    emitExpressionAddress(stm->target.get(), "%rdi");
    emitPush(asmReg(AsmRegister::RDI));
    emitExpressionAddress(stm->expression.get(), "%rsi");
    emitPop(asmReg(AsmRegister::RDI));
    emitCopyBytes(typeSize(targetType));
    return 0;
  }
  stm->expression->accept(this);
  storeTarget(target);
  return 0;
}

int GenCodeVisitor::visit(CompoundAssignmentStatement *stm) {
  const TypeInfo targetType = lvalueInfo(stm->target.get());
  const TypeInfo lhsType = targetType.decayed();
  const TypeInfo rhsType = expressionInfo(stm->expression.get()).decayed();
  const BinarySemantics semantics = analyzeBinary(stm->op, lhsType, rhsType);

  emitExpressionAddress(stm->target.get(), "%rdi");
  emitPush(asmReg(AsmRegister::RDI));
  emitLoadAddressed(targetType, indirectMem(AsmRegister::RDI));
  emitPush(asmReg(AsmRegister::RAX));
  stm->expression->accept(this);
  assembly.mov(AsmWidth::Quad, asmReg(AsmRegister::RAX),
               asmReg(AsmRegister::RCX));
  emitPop(asmReg(AsmRegister::RAX));

  if (stm->op == PLUS_OP && semantics.pointerArithmetic &&
      isPointerType(lhsType)) {
    const std::size_t step =
        std::max<std::size_t>(typeSize(pointedType(lhsType)), 1);
    if (step != 1)
      assembly.imul(AsmWidth::Quad, imm(static_cast<long>(step)),
                    asmReg(AsmRegister::RCX));
    assembly.add(AsmWidth::Quad, asmReg(AsmRegister::RCX),
                 asmReg(AsmRegister::RAX));
  } else if (stm->op == MINUS_OP && semantics.pointerArithmetic &&
             isPointerType(lhsType)) {
    const std::size_t step =
        std::max<std::size_t>(typeSize(pointedType(lhsType)), 1);
    if (step != 1)
      assembly.imul(AsmWidth::Quad, imm(static_cast<long>(step)),
                    asmReg(AsmRegister::RCX));
    assembly.sub(AsmWidth::Quad, asmReg(AsmRegister::RCX),
                 asmReg(AsmRegister::RAX));
  } else {
    const TypeInfo resultType = semantics.result;
    const AsmWidth width = expressionWidth(resultType);
    switch (stm->op) {
    case PLUS_OP:
      assembly.add(width, asmReg(AsmRegister::RCX, width),
                   asmReg(AsmRegister::RAX, width));
      break;
    case MINUS_OP:
      assembly.sub(width, asmReg(AsmRegister::RCX, width),
                   asmReg(AsmRegister::RAX, width));
      break;
    case MUL_OP:
      assembly.imul(width, asmReg(AsmRegister::RCX, width),
                    asmReg(AsmRegister::RAX, width));
      break;
    case DIV_OP:
      if (width == AsmWidth::Quad)
        assembly.cqto();
      else
        assembly.instr("cdq");
      assembly.idiv(width, asmReg(AsmRegister::RCX, width));
      break;
    case MOD_OP:
      if (width == AsmWidth::Quad)
        assembly.cqto();
      else
        assembly.instr("cdq");
      assembly.idiv(width, asmReg(AsmRegister::RCX, width));
      assembly.mov(width, asmReg(AsmRegister::RDX, width),
                   asmReg(AsmRegister::RAX, width));
      break;
    case LE_OP:
    case GT_OP:
    case LEQ_OP:
    case GEQ_OP:
    case EQ_OP:
    case NE_OP:
    case AND_OP:
    case OR_OP:
      throw std::runtime_error(
          "[CodeGen] Operador compuesto inválido en asignación");
    }
    normalizeScalarValue(resultType);
  }

  emitPop(asmReg(AsmRegister::RDI));
  emitStoreAddressed(targetType, indirectMem(AsmRegister::RDI));
  return 0;
}

int GenCodeVisitor::storeId(const LValue &lv) {
  emitStoreVariable(lv.name);
  return 0;
}

int GenCodeVisitor::storeSubscript(const LValue &lv) {
  emitPush(asmReg(AsmRegister::RAX));
  emitSubscriptAddress(lv);
  assembly.mov(AsmWidth::Quad, asmReg(AsmRegister::RAX),
               asmReg(AsmRegister::RDI));
  emitPop(asmReg(AsmRegister::RAX));
  TypeInfo target = lvalueInfo(lv.expression);
  emitStoreAddressed(target, indirectMem(AsmRegister::RDI));
  return 0;
}

int GenCodeVisitor::storeDeref(const LValue &lv) {
  emitPush(asmReg(AsmRegister::RAX));
  lv.expression->accept(this);
  assembly.mov(AsmWidth::Quad, asmReg(AsmRegister::RAX),
               asmReg(AsmRegister::RDI));
  emitPop(asmReg(AsmRegister::RAX));
  TypeInfo target = expressionInfo(lv.expression).decayed();
  if (isPointerType(target))
    target = pointedType(target);
  emitStoreAddressed(target, indirectMem(AsmRegister::RDI));
  return 0;
}

int GenCodeVisitor::storeField(const LValue &lv) {
  emitPush(asmReg(AsmRegister::RAX));
  emitExpressionAddress(lv.expression);
  emitPop(asmReg(AsmRegister::RCX));
  emitStoreAddressed(expressionInfo(lv.expression), indirectMem(),
                     AsmRegister::RCX);
  return 0;
}

int GenCodeVisitor::visit(BinaryExpression *exp) {
  if (exp->op == AND_OP) {
    const std::size_t lbl = labelCounter++;
    exp->left->accept(this);
    assembly.cmp(AsmWidth::Quad, imm(0), asmReg(AsmRegister::RAX));
    assembly.je("logical_false_" + std::to_string(lbl));
    exp->right->accept(this);
    assembly.cmp(AsmWidth::Quad, imm(0), asmReg(AsmRegister::RAX));
    assembly.je("logical_false_" + std::to_string(lbl));
    assembly.mov(AsmWidth::Quad, imm(1), asmReg(AsmRegister::RAX));
    assembly.jmp("logical_end_" + std::to_string(lbl));
    assembly.label("logical_false_" + std::to_string(lbl));
    assembly.mov(AsmWidth::Quad, imm(0), asmReg(AsmRegister::RAX));
    assembly.label("logical_end_" + std::to_string(lbl));
    return 0;
  }

  if (exp->op == OR_OP) {
    const std::size_t lbl = labelCounter++;
    exp->left->accept(this);
    assembly.cmp(AsmWidth::Quad, imm(0), asmReg(AsmRegister::RAX));
    assembly.jne("logical_true_" + std::to_string(lbl));
    exp->right->accept(this);
    assembly.cmp(AsmWidth::Quad, imm(0), asmReg(AsmRegister::RAX));
    assembly.jne("logical_true_" + std::to_string(lbl));
    assembly.mov(AsmWidth::Quad, imm(0), asmReg(AsmRegister::RAX));
    assembly.jmp("logical_end_" + std::to_string(lbl));
    assembly.label("logical_true_" + std::to_string(lbl));
    assembly.mov(AsmWidth::Quad, imm(1), asmReg(AsmRegister::RAX));
    assembly.label("logical_end_" + std::to_string(lbl));
    return 0;
  }

  exp->left->accept(this);
  emitPush(asmReg(AsmRegister::RAX));
  exp->right->accept(this);
  assembly.mov(AsmWidth::Quad, asmReg(AsmRegister::RAX),
               asmReg(AsmRegister::RCX));
  emitPop(asmReg(AsmRegister::RAX));
  const TypeInfo leftType = expressionInfo(exp->left.get()).decayed();
  const TypeInfo rightType = expressionInfo(exp->right.get()).decayed();
  const BinarySemantics semantics =
      analyzeBinary(exp->op, leftType, rightType,
                    isNullPointerConstant(exp->left.get()),
                    isNullPointerConstant(exp->right.get()));
  const bool pointerComparison =
      isPointerType(leftType) || isPointerType(rightType);
  const AsmWidth scalarWidth = semantics.integerArithmetic
                                   ? expressionWidth(semantics.result)
                                   : AsmWidth::Long;
  const AsmWidth comparisonWidth =
      pointerComparison
          ? AsmWidth::Quad
          : expressionWidth(usualArithmeticType(leftType, rightType));

  switch (exp->op) {
  case PLUS_OP:
    if (semantics.pointerArithmetic && isPointerType(leftType)) {
      const std::size_t step =
          std::max<std::size_t>(typeSize(pointedType(leftType)), 1);
      if (step != 1)
        assembly.imul(AsmWidth::Quad, imm(static_cast<long>(step)),
                      asmReg(AsmRegister::RCX));
      assembly.add(AsmWidth::Quad, asmReg(AsmRegister::RCX),
                   asmReg(AsmRegister::RAX));
    } else if (semantics.pointerArithmetic && isPointerType(rightType)) {
      const std::size_t step =
          std::max<std::size_t>(typeSize(pointedType(rightType)), 1);
      if (step != 1)
        assembly.imul(AsmWidth::Quad, imm(static_cast<long>(step)),
                      asmReg(AsmRegister::RAX));
      assembly.add(AsmWidth::Quad, asmReg(AsmRegister::RCX),
                   asmReg(AsmRegister::RAX));
    } else {
      assembly.add(scalarWidth, asmReg(AsmRegister::RCX, scalarWidth),
                   asmReg(AsmRegister::RAX, scalarWidth));
      normalizeScalarValue(semantics.result);
    }
    break;
  case MINUS_OP:
    if (semantics.pointerArithmetic && isPointerType(leftType)) {
      const std::size_t step =
          std::max<std::size_t>(typeSize(pointedType(leftType)), 1);
      if (step != 1)
        assembly.imul(AsmWidth::Quad, imm(static_cast<long>(step)),
                      asmReg(AsmRegister::RCX));
      assembly.sub(AsmWidth::Quad, asmReg(AsmRegister::RCX),
                   asmReg(AsmRegister::RAX));
    } else if (semantics.pointerDifference) {
      const std::size_t step =
          std::max<std::size_t>(typeSize(pointedType(leftType)), 1);
      assembly.sub(AsmWidth::Quad, asmReg(AsmRegister::RCX),
                   asmReg(AsmRegister::RAX));
      if (step != 1) {
        assembly.mov(AsmWidth::Quad, imm(static_cast<long>(step)),
                     asmReg(AsmRegister::RCX));
        assembly.cqto();
        assembly.idiv(AsmWidth::Quad, asmReg(AsmRegister::RCX));
      }
    } else {
      assembly.sub(scalarWidth, asmReg(AsmRegister::RCX, scalarWidth),
                   asmReg(AsmRegister::RAX, scalarWidth));
      normalizeScalarValue(semantics.result);
    }
    break;
  case MUL_OP:
    assembly.imul(scalarWidth, asmReg(AsmRegister::RCX, scalarWidth),
                  asmReg(AsmRegister::RAX, scalarWidth));
    normalizeScalarValue(semantics.result);
    break;
  case DIV_OP:
    if (scalarWidth == AsmWidth::Quad)
      assembly.cqto();
    else
      assembly.instr("cdq");
    assembly.idiv(scalarWidth, asmReg(AsmRegister::RCX, scalarWidth));
    normalizeScalarValue(semantics.result);
    break;
  case MOD_OP:
    if (scalarWidth == AsmWidth::Quad)
      assembly.cqto();
    else
      assembly.instr("cdq");
    assembly.idiv(scalarWidth, asmReg(AsmRegister::RCX, scalarWidth));
    assembly.mov(scalarWidth, asmReg(AsmRegister::RDX, scalarWidth),
                 asmReg(AsmRegister::RAX, scalarWidth));
    normalizeScalarValue(semantics.result);
    break;
  case LE_OP:
    assembly.cmp(comparisonWidth, asmReg(AsmRegister::RCX, comparisonWidth),
                 asmReg(AsmRegister::RAX, comparisonWidth));
    assembly.mov(AsmWidth::Quad, imm(0), asmReg(AsmRegister::RAX));
    assembly.set("l", asmReg(AsmRegister::RAX, AsmWidth::Byte));
    assembly.movzx(AsmWidth::Byte, AsmWidth::Quad,
                   asmReg(AsmRegister::RAX, AsmWidth::Byte),
                   asmReg(AsmRegister::RAX));
    break;
  case GT_OP:
    assembly.cmp(comparisonWidth, asmReg(AsmRegister::RCX, comparisonWidth),
                 asmReg(AsmRegister::RAX, comparisonWidth));
    assembly.mov(AsmWidth::Quad, imm(0), asmReg(AsmRegister::RAX));
    assembly.set("g", asmReg(AsmRegister::RAX, AsmWidth::Byte));
    assembly.movzx(AsmWidth::Byte, AsmWidth::Quad,
                   asmReg(AsmRegister::RAX, AsmWidth::Byte),
                   asmReg(AsmRegister::RAX));
    break;
  case LEQ_OP:
    assembly.cmp(comparisonWidth, asmReg(AsmRegister::RCX, comparisonWidth),
                 asmReg(AsmRegister::RAX, comparisonWidth));
    assembly.mov(AsmWidth::Quad, imm(0), asmReg(AsmRegister::RAX));
    assembly.set("le", asmReg(AsmRegister::RAX, AsmWidth::Byte));
    assembly.movzx(AsmWidth::Byte, AsmWidth::Quad,
                   asmReg(AsmRegister::RAX, AsmWidth::Byte),
                   asmReg(AsmRegister::RAX));
    break;
  case GEQ_OP:
    assembly.cmp(comparisonWidth, asmReg(AsmRegister::RCX, comparisonWidth),
                 asmReg(AsmRegister::RAX, comparisonWidth));
    assembly.mov(AsmWidth::Quad, imm(0), asmReg(AsmRegister::RAX));
    assembly.set("ge", asmReg(AsmRegister::RAX, AsmWidth::Byte));
    assembly.movzx(AsmWidth::Byte, AsmWidth::Quad,
                   asmReg(AsmRegister::RAX, AsmWidth::Byte),
                   asmReg(AsmRegister::RAX));
    break;
  case EQ_OP:
    assembly.cmp(comparisonWidth, asmReg(AsmRegister::RCX, comparisonWidth),
                 asmReg(AsmRegister::RAX, comparisonWidth));
    assembly.mov(AsmWidth::Quad, imm(0), asmReg(AsmRegister::RAX));
    assembly.set("e", asmReg(AsmRegister::RAX, AsmWidth::Byte));
    assembly.movzx(AsmWidth::Byte, AsmWidth::Quad,
                   asmReg(AsmRegister::RAX, AsmWidth::Byte),
                   asmReg(AsmRegister::RAX));
    break;
  case NE_OP:
    assembly.cmp(comparisonWidth, asmReg(AsmRegister::RCX, comparisonWidth),
                 asmReg(AsmRegister::RAX, comparisonWidth));
    assembly.mov(AsmWidth::Quad, imm(0), asmReg(AsmRegister::RAX));
    assembly.set("ne", asmReg(AsmRegister::RAX, AsmWidth::Byte));
    assembly.movzx(AsmWidth::Byte, AsmWidth::Quad,
                   asmReg(AsmRegister::RAX, AsmWidth::Byte),
                   asmReg(AsmRegister::RAX));
    break;
  case AND_OP:
  case OR_OP:
    break;
  }
  return 0;
}

int GenCodeVisitor::visit(UnaryExpression *exp) {
  exp->operand->accept(this);
  if (exp->op == NEG_OP) {
    const TypeInfo type = expressionInfo(exp);
    const AsmWidth width = expressionWidth(type);
    assembly.instr("neg", width, {asmReg(AsmRegister::RAX, width)});
    normalizeScalarValue(type);
    return 0;
  }
  const std::size_t lbl = labelCounter++;
  assembly.cmp(AsmWidth::Quad, imm(0), asmReg(AsmRegister::RAX));
  assembly.je("not_true_" + std::to_string(lbl));
  assembly.mov(AsmWidth::Quad, imm(0), asmReg(AsmRegister::RAX));
  assembly.jmp("not_end_" + std::to_string(lbl));
  assembly.label("not_true_" + std::to_string(lbl));
  assembly.mov(AsmWidth::Quad, imm(1), asmReg(AsmRegister::RAX));
  assembly.label("not_end_" + std::to_string(lbl));
  return 0;
}

int GenCodeVisitor::visit(IncDecExpression *exp) {
  const TypeInfo operandType = lvalueInfo(exp->operand.get());
  const TypeInfo decayed = operandType.decayed();
  const long step = isPointerType(decayed)
                        ? static_cast<long>(std::max<std::size_t>(
                              typeSize(pointedType(decayed)), 1))
                        : 1;

  emitExpressionAddress(exp->operand.get(), "%rdi");
  emitLoadAddressed(operandType, indirectMem(AsmRegister::RDI));
  if (exp->postfix)
    emitPush(asmReg(AsmRegister::RAX));

  const AsmWidth updateWidth =
      isPointerType(decayed) ? AsmWidth::Quad : expressionWidth(decayed);
  if (exp->op == IncDecOp::Increment)
    assembly.add(updateWidth, imm(step), asmReg(AsmRegister::RAX, updateWidth));
  else
    assembly.sub(updateWidth, imm(step), asmReg(AsmRegister::RAX, updateWidth));

  emitStoreAddressed(operandType, indirectMem(AsmRegister::RDI));
  if (exp->postfix)
    emitPop(asmReg(AsmRegister::RAX));
  else
    emitLoadAddressed(operandType, indirectMem(AsmRegister::RDI));
  return 0;
}

int GenCodeVisitor::visit(ExpressionStatement *stm) {
  stm->expression->accept(this);
  return 0;
}

int GenCodeVisitor::visit(Body *b) {
  const auto savedLocalOffsets = localOffsets;
  const auto savedInfos = variableInfos;
  const int savedOffset = offset;

  for (auto &item : b->items) {
    if (auto *declaration = dynamic_cast<DeclarationItem *>(item.get())) {
      declaration->declaration->accept(this);
    } else if (auto *statement = dynamic_cast<StatementItem *>(item.get())) {
      statement->statement->accept(this);
    }
  }

  localOffsets = savedLocalOffsets;
  variableInfos = savedInfos;
  offset = savedOffset;
  return 0;
}

int GenCodeVisitor::visit(IfStatement *stm) {
  const std::size_t lbl = labelCounter++;
  stm->condition->accept(this);
  assembly.cmp(AsmWidth::Quad, imm(0), asmReg(AsmRegister::RAX));
  assembly.je("else_" + std::to_string(lbl));
  stm->then->accept(this);
  assembly.jmp("endif_" + std::to_string(lbl));
  assembly.label("else_" + std::to_string(lbl));
  if (stm->elseBody)
    stm->elseBody->accept(this);
  assembly.label("endif_" + std::to_string(lbl));
  return 0;
}

int GenCodeVisitor::visit(ForStatement *stm) {
  const std::size_t lbl = labelCounter++;
  const auto savedLocalOffsets = localOffsets;
  const auto savedInfos = variableInfos;
  const int savedOffset = offset;
  const std::string oldBreak = currentBreakLabel;
  const std::string oldContinue = currentContinueLabel;
  const std::string conditionLabel = "for_condition_" + std::to_string(lbl);
  const std::string updateLabel = "for_update_" + std::to_string(lbl);
  const std::string endLabel = "endfor_" + std::to_string(lbl);

  currentBreakLabel = endLabel;
  currentContinueLabel = updateLabel;

  std::visit(
      [&](auto &initializer) {
        using Initializer = std::decay_t<decltype(initializer)>;
        if constexpr (!std::is_same_v<Initializer, std::monostate>) {
          if (initializer)
            initializer->accept(this);
        }
      },
      stm->initializer);

  assembly.label(conditionLabel);
  if (stm->condition) {
    stm->condition->accept(this);
    assembly.cmp(AsmWidth::Quad, imm(0), asmReg(AsmRegister::RAX));
    assembly.je(endLabel);
  }

  stm->body->accept(this);
  assembly.label(updateLabel);
  if (stm->update)
    stm->update->accept(this);
  assembly.jmp(conditionLabel);
  assembly.label(endLabel);

  currentBreakLabel = oldBreak;
  currentContinueLabel = oldContinue;
  localOffsets = savedLocalOffsets;
  variableInfos = savedInfos;
  offset = savedOffset;
  return 0;
}

int GenCodeVisitor::visit(WhileStatement *stm) {
  const std::size_t lbl = labelCounter++;
  std::string oldBreak = currentBreakLabel;
  std::string oldContinue = currentContinueLabel;
  currentBreakLabel = "endwhile_" + std::to_string(lbl);
  currentContinueLabel = "while_" + std::to_string(lbl);

  assembly.label("while_" + std::to_string(lbl));
  stm->condition->accept(this);
  assembly.cmp(AsmWidth::Quad, imm(0), asmReg(AsmRegister::RAX));
  assembly.je("endwhile_" + std::to_string(lbl));
  stm->body->accept(this);
  assembly.jmp("while_" + std::to_string(lbl));
  assembly.label("endwhile_" + std::to_string(lbl));

  currentBreakLabel = oldBreak;
  currentContinueLabel = oldContinue;
  return 0;
}

int GenCodeVisitor::visit(DoWhileStatement *stm) {
  const std::size_t lbl = labelCounter++;
  std::string oldBreak = currentBreakLabel;
  std::string oldContinue = currentContinueLabel;
  currentBreakLabel = "endwhile_" + std::to_string(lbl);
  currentContinueLabel = "dowhile_continue_" + std::to_string(lbl);

  assembly.label("dowhile_" + std::to_string(lbl));
  stm->body->accept(this);
  assembly.label("dowhile_continue_" + std::to_string(lbl));
  stm->condition->accept(this);
  assembly.cmp(AsmWidth::Quad, imm(0), asmReg(AsmRegister::RAX));
  assembly.jne("dowhile_" + std::to_string(lbl));
  assembly.label("endwhile_" + std::to_string(lbl));

  currentBreakLabel = oldBreak;
  currentContinueLabel = oldContinue;
  return 0;
}

int GenCodeVisitor::visit(ReturnStatement *stm) {
  if (stm->expression) {
    const TypeInfo returnType = functionReturnTypes.at(currentFunctionName);
    if (returnType.isStructObject()) {
      const AbiInfo abi = classifyAbi(returnType, structInfos);
      if (abi.memory) {
        assembly.mov(AsmWidth::Quad, stackMem(hiddenReturnOffset),
                     asmReg(AsmRegister::RDI));
        assembly.mov(AsmWidth::Quad, asmReg(AsmRegister::RDI),
                     asmReg(AsmRegister::RAX));
        emitExpressionAddress(stm->expression.get(), "%rsi");
        emitCopyBytes(typeSize(returnType));
      } else {
        for (std::size_t i = abi.slots.size(); i > 0; --i) {
          const std::size_t index = i - 1;
          emitLoadAggregateSlot(stm->expression.get(), abi.slots[index].offset,
                                abi.slots[index].bytes,
                                returnRegisters[index]);
        }
      }
      assembly.jmp(".end_" + currentFunctionName);
      return 0;
    }
    stm->expression->accept(this);
    normalizeReturnValue(returnType);
  }
  assembly.jmp(".end_" + currentFunctionName);
  return 0;
}

int GenCodeVisitor::visit(BreakStatement *) {
  if (currentBreakLabel.empty())
    throw std::runtime_error(
        "[CodeGen] break sin etiqueta de salida disponible");
  assembly.jmp(currentBreakLabel);
  return 0;
}

int GenCodeVisitor::visit(ContinueStatement *) {
  if (currentContinueLabel.empty())
    throw std::runtime_error(
        "[CodeGen] continue sin etiqueta de continuación disponible");
  assembly.jmp(currentContinueLabel);
  return 0;
}

int GenCodeVisitor::visit(SwitchStatement *stm) {
  const std::size_t lbl = labelCounter++;
  stm->expression->accept(this);
  assembly.mov(AsmWidth::Quad, asmReg(AsmRegister::RAX),
               asmReg(AsmRegister::R10));

  std::size_t caseIndex = 0;
  for (auto &c : stm->cases) {
    assembly.mov(AsmWidth::Quad, imm(c->value), asmReg(AsmRegister::RAX));
    assembly.cmp(AsmWidth::Quad, asmReg(AsmRegister::RAX),
                 asmReg(AsmRegister::R10));
    assembly.je("case_" + std::to_string(lbl) + "_" +
                std::to_string(caseIndex));
    caseIndex++;
  }

  if (!stm->defaultBody.empty())
    assembly.jmp("default_" + std::to_string(lbl));
  else
    assembly.jmp("endswitch_" + std::to_string(lbl));

  std::string oldBreak = currentBreakLabel;
  currentBreakLabel = "endswitch_" + std::to_string(lbl);

  caseIndex = 0;
  for (auto &c : stm->cases) {
    assembly.label("case_" + std::to_string(lbl) + "_" +
                   std::to_string(caseIndex));
    for (auto &s : c->body)
      s->accept(this);
    caseIndex++;
  }

  if (!stm->defaultBody.empty()) {
    assembly.label("default_" + std::to_string(lbl));
    for (auto &s : stm->defaultBody)
      s->accept(this);
  }

  currentBreakLabel = oldBreak;
  assembly.label("endswitch_" + std::to_string(lbl));
  return 0;
}

int GenCodeVisitor::visit(FunctionDeclaration *f) {
  insideFunction = true;
  localOffsets.clear();
  variableInfos = globalVariableInfos;
  offset = 0;
  currentFunctionName = f->name;
  const TypeInfo returnType = functionReturnTypes.at(f->name);
  const bool functionReturnsInMemory = returnsViaHiddenPointer(returnType);

  assembly.blank();
  assembly.directive(".globl " + f->name);
  assembly.label(f->name);
  assembly.push(asmReg(AsmRegister::RBP));
  assembly.mov(AsmWidth::Quad, asmReg(AsmRegister::RSP),
               asmReg(AsmRegister::RBP));
  const std::size_t hiddenBytes = functionReturnsInMemory ? 8 : 0;
  assembly.sub(AsmWidth::Quad,
               imm(static_cast<long>(
                   alignTo(functionFrameBytes[f->name] + hiddenBytes, 16))),
               asmReg(AsmRegister::RSP));
  evalStackBytes = 0;
  if (functionReturnsInMemory) {
    offset -= 8;
    hiddenReturnOffset = offset;
    assembly.mov(AsmWidth::Quad, asmReg(AsmRegister::RDI),
                 stackMem(hiddenReturnOffset));
  }

  const std::size_t nParams = f->parameterNames.size();
  std::vector<TypeInfo> parameterTypes;
  std::vector<int> parameterOffsets;
  parameterTypes.reserve(nParams);
  parameterOffsets.reserve(nParams);
  for (std::size_t i = 0; i < nParams; i++) {
    const TypeInfo parameterType{
        f->parameterBaseTypes[i], f->parameterPointerDepths[i], {}};
    const auto layout = layoutOf(parameterType, structInfos);
    offset -= static_cast<int>(layout.size);
    const int remainder = (-offset) % static_cast<int>(layout.align);
    if (remainder != 0)
      offset -= static_cast<int>(layout.align) - remainder;
    localOffsets[f->parameterNames[i]] = offset;
    variableInfos[f->parameterNames[i]] = parameterType;
    parameterTypes.push_back(parameterType);
    parameterOffsets.push_back(offset);
  }

  struct ParameterPlan {
    TypeInfo type;
    AbiInfo abi;
    std::vector<int> registers;
    std::size_t stackOffset = 0;
    std::size_t stackBytes = 0;
  };

  std::vector<ParameterPlan> parameterPlans;
  parameterPlans.reserve(nParams);
  std::size_t nextRegister = functionReturnsInMemory ? 1 : 0;
  long callerOffset = 16;
  for (std::size_t i = 0; i < nParams; i++) {
    ParameterPlan plan;
    plan.type = parameterTypes[i];
    plan.abi = classifyAbi(plan.type, structInfos);
    const std::size_t neededRegisters = plan.abi.slots.size();
    if (plan.abi.memory) {
      plan.stackOffset = static_cast<std::size_t>(callerOffset);
      plan.stackBytes = alignTo(typeSize(plan.type), 8);
      callerOffset += static_cast<long>(plan.stackBytes);
    } else if (nextRegister + neededRegisters <= argRegisters.size()) {
      for (std::size_t slot = 0; slot < neededRegisters; ++slot)
        plan.registers.push_back(static_cast<int>(nextRegister++));
    } else {
      plan.stackOffset = static_cast<std::size_t>(callerOffset);
      plan.stackBytes =
          plan.type.isStructObject() ? alignTo(typeSize(plan.type), 8) : 8;
      callerOffset += static_cast<long>(plan.stackBytes);
    }
    parameterPlans.push_back(std::move(plan));
  }

  int highestRegister = -1;
  for (const ParameterPlan &plan : parameterPlans)
    for (int reg : plan.registers)
      highestRegister = std::max(highestRegister, reg);

  if (highestRegister >= 0) {
    for (int reg = 0; reg <= highestRegister; ++reg)
      emitPush(asmReg(argRegisters[reg]));

    auto savedRegisterOffset = [&](int reg) {
      return static_cast<long>((highestRegister - reg) * 8);
    };

    for (std::size_t i = 0; i < nParams; i++) {
      const ParameterPlan &plan = parameterPlans[i];
      if (plan.registers.empty())
        continue;
      if (plan.type.isStructObject()) {
        for (std::size_t slot = 0; slot < plan.abi.slots.size(); ++slot) {
          const AbiSlot &abiSlot = plan.abi.slots[slot];
          assembly.lea(stackMem(parameterOffsets[i] +
                                static_cast<int>(abiSlot.offset)),
                       asmReg(AsmRegister::RDI));
          assembly.lea(indirectMem(AsmRegister::RSP,
                                   savedRegisterOffset(plan.registers[slot])),
                       asmReg(AsmRegister::RSI));
          emitCopyBytes(abiSlot.bytes);
        }
      } else {
        emitLoadAddressed(
            plan.type,
            indirectMem(AsmRegister::RSP,
                        savedRegisterOffset(plan.registers[0])));
        emitStoreAddressed(plan.type, stackMem(parameterOffsets[i]));
      }
    }

    const std::size_t savedBytes =
        static_cast<std::size_t>(highestRegister + 1) * 8;
    assembly.add(AsmWidth::Quad, imm(static_cast<long>(savedBytes)),
                 asmReg(AsmRegister::RSP));
    if (evalStackBytes < savedBytes)
      throw std::runtime_error("[CodeGen] Desbalance de pila temporal");
    evalStackBytes -= savedBytes;
  }

  for (std::size_t i = 0; i < nParams; i++) {
    const ParameterPlan &plan = parameterPlans[i];
    if (plan.stackBytes != 0) {
      if (plan.type.isStructObject()) {
        assembly.lea(stackMem(parameterOffsets[i]), asmReg(AsmRegister::RDI));
        assembly.lea(stackMem(static_cast<long>(plan.stackOffset)),
                     asmReg(AsmRegister::RSI));
        emitCopyBytes(typeSize(plan.type));
      } else {
        emitLoadAddressed(plan.type,
                          stackMem(static_cast<long>(plan.stackOffset)));
        emitStoreAddressed(plan.type, stackMem(parameterOffsets[i]));
      }
    }
  }

  f->body->accept(this);

  assembly.label(".end_" + f->name);
  assembly.instr("leave");
  assembly.ret();

  insideFunction = false;
  hiddenReturnOffset = 0;
  return 0;
}

void GenCodeVisitor::emitUserFunctionCall(FunctionCallExpression *exp,
                                          bool hiddenStructReturn) {
  struct ArgumentPlan {
    TypeInfo type;
    AbiInfo abi;
    std::vector<int> registers;
    std::size_t stackBytes = 0;
  };

  const std::size_t nArgs = exp->arguments.size();
  const auto &paramTypes = functionParameterTypes.at(exp->name);
  const bool variadic = functionVariadicFlags.at(exp->name);
  const std::size_t fixedArgs = paramTypes.size();
  auto argumentType = [&](std::size_t index) {
    if (index < fixedArgs)
      return paramTypes[index];
    TypeInfo type = expressionInfo(exp->arguments[index].get());
    if (variadic)
      return defaultArgumentPromotion(type);
    return type.decayed();
  };
  std::vector<ArgumentPlan> plans;
  plans.reserve(nArgs);
  std::size_t nextRegister = hiddenStructReturn ? 1 : 0;
  std::size_t totalStackBytes = 0;

  for (std::size_t i = 0; i < nArgs; ++i) {
    ArgumentPlan plan;
    plan.type = argumentType(i).decayed();
    plan.abi = classifyAbi(plan.type, structInfos);
    const std::size_t neededRegisters = plan.abi.slots.size();
    if (plan.abi.memory) {
      plan.stackBytes = alignTo(typeSize(plan.type), 8);
      totalStackBytes += plan.stackBytes;
    } else if (nextRegister + neededRegisters <= argRegisters.size()) {
      for (std::size_t slot = 0; slot < neededRegisters; ++slot)
        plan.registers.push_back(static_cast<int>(nextRegister++));
    } else {
      plan.stackBytes =
          plan.type.isStructObject() ? alignTo(typeSize(plan.type), 8) : 8;
      totalStackBytes += plan.stackBytes;
    }
    plans.push_back(std::move(plan));
  }

  std::size_t outgoingPadding = 0;
  if (totalStackBytes > 0 && (evalStackBytes + totalStackBytes) % 16 != 0) {
    assembly.sub(AsmWidth::Quad, imm(8), asmReg(AsmRegister::RSP));
    evalStackBytes += 8;
    outgoingPadding = 8;
  }

  for (std::size_t i = nArgs; i > 0; --i) {
    const std::size_t index = i - 1;
    const ArgumentPlan &plan = plans[index];
    if (plan.stackBytes == 0)
      continue;
    if (plan.type.isStructObject()) {
      emitExpressionAddress(exp->arguments[index].get(), "%rsi");
      assembly.sub(AsmWidth::Quad, imm(static_cast<long>(plan.stackBytes)),
                   asmReg(AsmRegister::RSP));
      evalStackBytes += plan.stackBytes;
      assembly.lea(AsmOperand::indirect(AsmRegister::RSP),
                   asmReg(AsmRegister::RDI));
      emitCopyBytes(typeSize(plan.type));
    } else {
      exp->arguments[index]->accept(this);
      emitPush(asmReg(AsmRegister::RAX));
    }
  }

  for (std::size_t i = 0; i < nArgs; ++i) {
    const ArgumentPlan &plan = plans[i];
    if (plan.registers.empty())
      continue;
    if (plan.type.isStructObject()) {
      for (const AbiSlot &slot : plan.abi.slots) {
        emitLoadAggregateSlot(exp->arguments[i].get(), slot.offset, slot.bytes,
                              AsmRegister::RAX);
        emitPush(asmReg(AsmRegister::RAX));
      }
    } else {
      exp->arguments[i]->accept(this);
      emitPush(asmReg(AsmRegister::RAX));
    }
  }

  for (std::size_t i = nArgs; i > 0; --i) {
    const std::size_t index = i - 1;
    const ArgumentPlan &plan = plans[index];
    if (plan.registers.empty())
      continue;
    for (std::size_t slot = plan.registers.size(); slot > 0; --slot) {
      const std::size_t slotIndex = slot - 1;
      emitPop(asmReg(AsmRegister::RAX));
      if (plan.type.isStructObject())
        assembly.mov(AsmWidth::Quad, asmReg(AsmRegister::RAX),
                     asmReg(argRegisters[plan.registers[slotIndex]]));
      else
        emitMoveArgumentToRegister(plan.type,
                                   argRegisters[plan.registers[slotIndex]]);
    }
  }

  if (hiddenStructReturn) {
    if (!hiddenStructReturnTarget)
      throw std::runtime_error("[CodeGen] Retorno struct sin destino oculto");
    emitExpressionAddress(hiddenStructReturnTarget, "%rdi");
  }

  if (variadic)
    assembly.mov(AsmWidth::Quad, imm(0), asmReg(AsmRegister::RAX));
  emitCall(exp->name);
  if (totalStackBytes > 0) {
    const std::size_t outgoingBytes = totalStackBytes + outgoingPadding;
    assembly.add(AsmWidth::Quad, imm(static_cast<long>(outgoingBytes)),
                 asmReg(AsmRegister::RSP));
    if (evalStackBytes < outgoingBytes)
      throw std::runtime_error("[CodeGen] Desbalance de arguments en pila");
    evalStackBytes -= outgoingBytes;
  }
}

int GenCodeVisitor::visit(FunctionCallExpression *exp) {
  if (functionParameterTypes.contains(exp->name)) {
    if (expressionInfo(exp).isStructObject())
      throw std::runtime_error(
          "[CodeGen] Retorno struct solo soportado en asignación directa");
    emitUserFunctionCall(exp, false);
    normalizeScalarValue(expressionInfo(exp));
    return 0;
  }

  throw std::runtime_error("[CodeGen] Función no definida: '" + exp->name +
                           "'");
}
