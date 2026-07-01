#include "../include/visitor_utils.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>

SemanticInfo TypeCheckerVisitor::analyze(Program *program) {
  symbolScopes.clear();
  variableTypes.clear();
  functionReturnTypes.clear();
  functionParameterTypes.clear();
  functionVariadicFlags.clear();
  functionDefinedFlags.clear();
  structInfos.clear();
  breakableDepth = 0;
  loopDepth = 0;

  for (auto &sd : program->structDeclarations) {
    if (structInfos.count(sd->name))
      throw std::runtime_error("[TypeChecker] Estructura duplicada: '" +
                               sd->name + "'");
    structInfos[sd->name] = StructInfo{};
  }

  for (auto &sd : program->structDeclarations)
    sd->accept(this);

  symbolScopes.add_level();
  variableTypes.add_level();

  for (auto &vd : program->globalDeclarations) {
    requireKnownType(vd->baseType, "declaración global");
    for (auto &declarator : vd->declarators) {
      if (symbolScopes.check_current(declarator.name))
        throw std::runtime_error("[TypeChecker] Símbolo global duplicado: '" +
                                 declarator.name + "'");
      if (functionReturnTypes.count(declarator.name))
        throw std::runtime_error(
            "[TypeChecker] Variable global conflictúa con función: '" +
            declarator.name + "'");
      const auto type = typeFromDeclarator(vd->baseType, declarator);
      if (isVoidObject(type))
        throw std::runtime_error(
            "[TypeChecker] No se puede declarar objeto de tipo void");
      if (declarator.initializer) {
        if (type.isArray())
          throw std::runtime_error(
              "[TypeChecker] Los arreglos globales no soportan inicializador");
        if (type.isStructObject())
          throw std::runtime_error(
              "[TypeChecker] Las estructuras globales no soportan "
              "inicializador");
        auto *number =
            dynamic_cast<NumberExpression *>(declarator.initializer.get());
        auto *string =
            dynamic_cast<StringExpression *>(declarator.initializer.get());
        if (!number && !string)
          throw std::runtime_error(
              "[TypeChecker] Inicializador global debe ser constante numérica "
              "o literal de cadena");
        if (number && isPointerType(type) && number->value != 0)
          throw std::runtime_error(
              "[TypeChecker] Inicializador global de puntero debe ser 0");
        requireAssignableExpression(type, declarator.initializer.get(),
                                    "inicialización global de '" +
                                        declarator.name + "'");
      }
      symbolScopes.add_var(declarator.name, 0);
      variableTypes.add_var(declarator.name, type);
    }
  }

  for (auto &fd : program->functionDeclarations) {
    if (symbolScopes.check(fd->name))
      throw std::runtime_error(
          "[TypeChecker] Función conflictúa con variable global: '" + fd->name +
          "'");
    requireKnownType(fd->returnBaseType,
                     "retorno de función '" + fd->name + "'");
    const TypeInfo returnType{fd->returnBaseType, fd->returnPointerDepth, {}};
    std::vector<TypeInfo> params;
    std::unordered_map<std::string, bool> paramNames;
    for (std::size_t i = 0; i < fd->parameterBaseTypes.size(); ++i) {
      if (fd->parameterNames[i].empty() && fd->hasBody())
        throw std::runtime_error("[TypeChecker] Parámetro sin nombre en "
                                 "definición de función '" +
                                 fd->name + "'");
      if (!fd->parameterNames[i].empty() &&
          paramNames.count(fd->parameterNames[i]))
        throw std::runtime_error("[TypeChecker] Parámetro duplicado '" +
                                 fd->parameterNames[i] + "' en función '" +
                                 fd->name + "'");
      if (!fd->parameterNames[i].empty())
        paramNames[fd->parameterNames[i]] = true;
      requireKnownType(fd->parameterBaseTypes[i],
                       "parámetro '" + fd->parameterNames[i] +
                           "' de función '" + fd->name + "'");
      if (fd->parameterBaseTypes[i] == "void" &&
          fd->parameterPointerDepths[i] == 0)
        throw std::runtime_error(
            "[TypeChecker] Los parámetros no pueden tener tipo void");
      params.push_back(TypeInfo{
          fd->parameterBaseTypes[i], fd->parameterPointerDepths[i], {}});
    }

    if (functionReturnTypes.contains(fd->name)) {
      if (!sameSignature(fd.get(), functionReturnTypes.at(fd->name),
                         functionParameterTypes.at(fd->name)))
        throw std::runtime_error(
            "[TypeChecker] Declaración incompatible de función '" + fd->name +
            "'");
    } else {
      functionReturnTypes[fd->name] = returnType;
      functionParameterTypes[fd->name] = params;
      functionVariadicFlags[fd->name] = fd->variadic;
      functionDefinedFlags[fd->name] = false;
    }

    if (fd->hasBody()) {
      if (functionDefinedFlags.at(fd->name))
        throw std::runtime_error("[TypeChecker] Función duplicada: '" +
                                 fd->name + "'");
      functionDefinedFlags[fd->name] = true;
    }
  }

  for (auto &fd : program->functionDeclarations)
    if (fd->hasBody())
      fd->accept(this);

  return SemanticInfo{functionFrameBytes, structInfos, functionReturnTypes,
                      functionParameterTypes, functionVariadicFlags};
}

bool TypeCheckerVisitor::isKnownType(const std::string &type) const {
  return isBuiltinType(type) || structInfos.contains(type);
}

bool TypeCheckerVisitor::sameSignature(
    const FunctionDeclaration *function, const TypeInfo &returnType,
    const std::vector<TypeInfo> &params) const {
  const TypeInfo candidateReturn{
      function->returnBaseType, function->returnPointerDepth, {}};
  if (!candidateReturn.same(returnType))
    return false;
  if (function->variadic != functionVariadicFlags.at(function->name))
    return false;
  if (function->parameterBaseTypes.size() != params.size())
    return false;
  for (std::size_t i = 0; i < function->parameterBaseTypes.size(); ++i) {
    const TypeInfo candidateParam{function->parameterBaseTypes[i],
                                  function->parameterPointerDepths[i],
                                  {}};
    if (!candidateParam.same(params[i]))
      return false;
  }
  return true;
}

TypeInfo
TypeCheckerVisitor::typeFromDeclarator(const std::string &base,
                                       const Declarator &declarator) const {
  return TypeInfo{base, declarator.pointerDepth,
                  constDimensions(declarator.dimensions)};
}

void TypeCheckerVisitor::requireAssignable(const TypeInfo &target,
                                           const TypeInfo &source,
                                           const std::string &context) const {
  if (!assignmentConversion(target, source).valid())
    throw std::runtime_error("[TypeChecker] No se puede asignar " +
                             source.display() + " a " + target.display() +
                             " en " + context);
}

void TypeCheckerVisitor::requireAssignableExpression(
    const TypeInfo &target, Expression *source,
    const std::string &context) const {
  const auto sourceType = infer(source);
  if (!assignmentConversion(target, sourceType, isNullPointerConstant(source))
           .valid())
    throw std::runtime_error("[TypeChecker] No se puede asignar " +
                             sourceType.display() + " a " +
                             target.display() + " en " + context);
}

void TypeCheckerVisitor::requireUserFunctionCall(
    const FunctionCallExpression *call) const {
  if (!functionParameterTypes.contains(call->name))
    throw std::runtime_error("[TypeChecker] Función no definida: '" +
                             call->name + "' llamada en '" + currentFunction +
                             "'");

  const auto &params = functionParameterTypes.at(call->name);
  const std::size_t expected = params.size();
  const std::size_t received = call->arguments.size();
  const bool variadic = functionVariadicFlags.at(call->name);
  if ((!variadic && received != expected) || (variadic && received < expected))
    throw std::runtime_error("[TypeChecker] La función '" + call->name +
                             "' espera " + std::to_string(expected) +
                             " argumento(s), pero se pasaron " +
                             std::to_string(received));

  for (std::size_t i = 0; i < expected; ++i) {
    if (params[i].isStructObject() &&
        !isLvalueExpression(call->arguments[i].get()))
      throw std::runtime_error(
          "[TypeChecker] Argumento struct por valor debe ser objeto "
          "direccionable");
    requireAssignableExpression(params[i], call->arguments[i].get(),
                                "argumento de '" + call->name + "'");
  }

  for (std::size_t i = expected; i < call->arguments.size(); ++i) {
    const auto argumentType =
        defaultArgumentPromotion(infer(call->arguments[i].get()));
    if (!isScalarInteger(argumentType) && !isPointerType(argumentType))
      throw std::runtime_error("[TypeChecker] Argumento variádico de '" +
                               call->name + "' debe ser entero o puntero");
  }
}

void TypeCheckerVisitor::requireCondition(Expression *exp,
                                          const std::string &context) const {
  TypeInfo type = infer(exp);
  if (!conditionConversion(type).valid())
    throw std::runtime_error("[TypeChecker] " + context +
                             " requiere expresión int o puntero");
}

void TypeCheckerVisitor::requireInteger(Expression *exp,
                                        const std::string &context) const {
  TypeInfo type;
  if (dynamic_cast<IdentifierExpression *>(exp) ||
      dynamic_cast<SubscriptExpression *>(exp) ||
      dynamic_cast<DereferenceExpression *>(exp) ||
      dynamic_cast<FieldExpression *>(exp)) {
    type = inferLvalue(exp);
  } else {
    type = infer(exp);
  }
  if (!isScalarInteger(type))
    throw std::runtime_error("[TypeChecker] " + context +
                             " requiere expresión int");
}

TypeInfo TypeCheckerVisitor::inferLvalue(Expression *exp) const {
  TypeResolver resolver(
      [&](const std::string &name) { return variableTypes.lookup(name); },
      [&](const std::string &name) -> std::optional<TypeInfo> {
        if (!functionReturnTypes.contains(name))
          return std::nullopt;
        return functionReturnTypes.at(name);
      },
      [&](const FunctionCallExpression *call) { requireUserFunctionCall(call); },
      structInfos, currentFunction);
  return resolver.lvalue(exp);
}

TypeInfo TypeCheckerVisitor::infer(Expression *exp) const {
  TypeResolver resolver(
      [&](const std::string &name) { return variableTypes.lookup(name); },
      [&](const std::string &name) -> std::optional<TypeInfo> {
        if (!functionReturnTypes.contains(name))
          return std::nullopt;
        return functionReturnTypes.at(name);
      },
      [&](const FunctionCallExpression *call) { requireUserFunctionCall(call); },
      structInfos, currentFunction);
  return resolver.expression(exp);
}

void TypeCheckerVisitor::requireKnownType(const std::string &type,
                                          const std::string &context) const {
  if (!isKnownType(type))
    throw std::runtime_error("[TypeChecker] Tipo no definido '" + type +
                             "' en " + context);
}

bool TypeCheckerVisitor::statementListAlwaysReturns(
    const std::list<std::unique_ptr<Statement>> &statements) const {
  return std::ranges::any_of(statements, [&](const auto &statement) {
    return statementAlwaysReturns(statement.get());
  });
}

bool TypeCheckerVisitor::bodyAlwaysReturns(const Body *body) const {
  return std::ranges::any_of(body->items, [&](const auto &item) {
    auto *statement = dynamic_cast<StatementItem *>(item.get());
    return statement && statementAlwaysReturns(statement->statement.get());
  });
}

bool TypeCheckerVisitor::switchAlwaysReturns(const SwitchStatement *stm) const {
  if (stm->defaultBody.empty())
    return false;

  bool suffixReturns = statementListAlwaysReturns(stm->defaultBody);
  for (auto it = stm->cases.rbegin(); it != stm->cases.rend(); ++it) {
    suffixReturns = statementListAlwaysReturns((*it)->body) || suffixReturns;
    if (!suffixReturns)
      return false;
  }
  return suffixReturns;
}

bool TypeCheckerVisitor::statementAlwaysReturns(const Statement *stm) const {
  if (dynamic_cast<const ReturnStatement *>(stm))
    return true;

  if (auto *ifStm = dynamic_cast<const IfStatement *>(stm))
    return ifStm->elseBody && bodyAlwaysReturns(ifStm->then.get()) &&
           bodyAlwaysReturns(ifStm->elseBody.get());

  if (auto *doWhileStm = dynamic_cast<const DoWhileStatement *>(stm))
    return bodyAlwaysReturns(doWhileStm->body.get());

  if (auto *forStm = dynamic_cast<const ForStatement *>(stm))
    return !forStm->condition && bodyAlwaysReturns(forStm->body.get());

  if (auto *switchStm = dynamic_cast<const SwitchStatement *>(stm))
    return switchAlwaysReturns(switchStm);

  return false;
}

int TypeCheckerVisitor::visit(FunctionDeclaration *fd) {
  currentFunction = fd->name;
  currentReturnType = functionReturnTypes.at(fd->name);
  localBytes = 0;
  std::size_t parameterBytes = 0;

  symbolScopes.add_level();
  variableTypes.add_level();

  std::unordered_map<std::string, bool> parameterNames;
  for (std::size_t i = 0; i < fd->parameterNames.size(); ++i) {
    if (symbolScopes.check_current(fd->parameterNames[i]))
      throw std::runtime_error("[TypeChecker] Parámetro duplicado '" +
                               fd->parameterNames[i] + "' en función '" +
                               fd->name + "'");
    parameterNames[fd->parameterNames[i]] = true;
    symbolScopes.add_var(fd->parameterNames[i], 0);
    const TypeInfo parameterType = functionParameterTypes.at(fd->name)[i];
    const auto layout = layoutOf(parameterType, structInfos);
    parameterBytes += layout.size;
    parameterBytes = alignTo(parameterBytes, layout.align);
    variableTypes.add_var(fd->parameterNames[i], parameterType);
  }

  for (auto &item : fd->body->items) {
    auto *declaration = dynamic_cast<DeclarationItem *>(item.get());
    if (!declaration)
      continue;
    for (auto &declarator : declaration->declaration->declarators)
      if (parameterNames.count(declarator.name))
        throw std::runtime_error("[TypeChecker] Variable '" + declarator.name +
                                 "' redeclara un parámetro en función '" +
                                 fd->name + "'");
  }

  fd->body->accept(this);
  const bool returnsVoid = currentReturnType.isVoidObject();
  if (!returnsVoid && !bodyAlwaysReturns(fd->body.get()))
    throw std::runtime_error("[TypeChecker] La función '" + fd->name +
                             "' puede terminar sin return");

  variableTypes.remove_level();
  symbolScopes.remove_level();

  functionFrameBytes[fd->name] = parameterBytes + localBytes;
  return 0;
}

int TypeCheckerVisitor::visit(Body *body) {
  symbolScopes.add_level();
  variableTypes.add_level();

  for (auto &item : body->items) {
    if (auto *declaration = dynamic_cast<DeclarationItem *>(item.get())) {
      declaration->declaration->accept(this);
    } else if (auto *statement = dynamic_cast<StatementItem *>(item.get())) {
      statement->statement->accept(this);
    }
  }

  variableTypes.remove_level();
  symbolScopes.remove_level();
  return 0;
}

int TypeCheckerVisitor::visit(VariableDeclaration *vd) {
  requireKnownType(vd->baseType, "declaración de variable");
  for (auto &declarator : vd->declarators) {
    const auto &name = declarator.name;
    const auto type = typeFromDeclarator(vd->baseType, declarator);
    if (isVoidObject(type))
      throw std::runtime_error(
          "[TypeChecker] No se puede declarar objeto de tipo void");
    if (symbolScopes.check_current(name))
      throw std::runtime_error("[TypeChecker] Variable duplicada en scope: '" +
                               name + "'");
    symbolScopes.add_var(name, 0);
    variableTypes.add_var(name, type);
    const auto layout = layoutOf(type, structInfos);
    localBytes += layout.size;
    localBytes = alignTo(localBytes, layout.align);
    if (declarator.initializer) {
      if (type.isArray())
        throw std::runtime_error(
            "[TypeChecker] Los arreglos no soportan inicializador directo");
      if (type.isStructObject())
        throw std::runtime_error(
            "[TypeChecker] Las estructuras no soportan inicializador directo");
      requireAssignableExpression(type, declarator.initializer.get(),
                                  "inicialización de '" + name + "'");
    }
  }
  return 0;
}

int TypeCheckerVisitor::visit(StructDeclaration *sd) {
  std::size_t offset = 0;
  std::size_t structAlign = 1;
  std::unordered_map<std::string, bool> names;
  StructInfo info;
  for (auto &field : sd->fields) {
    requireKnownType(field->baseType, "campo de estructura '" + sd->name + "'");
    for (auto &declarator : field->declarators) {
      const auto &name = declarator.name;
      if (names.count(name))
        throw std::runtime_error("[TypeChecker] Campo duplicado '" + name +
                                 "' en estructura '" + sd->name + "'");
      const auto type = typeFromDeclarator(field->baseType, declarator);
      if (isVoidObject(type))
        throw std::runtime_error(
            "[TypeChecker] Campo de estructura no puede tener tipo void");
      if (type.isStructObject() && structInfos[type.name].size == 0)
        throw std::runtime_error(
            "[TypeChecker] Campo de estructura usa estructura incompleta");
      const auto layout = layoutOf(type, structInfos);
      offset = alignTo(offset, layout.align);
      names[name] = true;
      info.fields[name] = FieldInfo{type, offset};
      offset += layout.size;
      structAlign = std::max(structAlign, layout.align);
    }
  }

  info.size = alignTo(offset, structAlign);
  info.align = structAlign;
  structInfos[sd->name] = info;
  return 0;
}

int TypeCheckerVisitor::visit(IdentifierExpression *exp) {
  if (!symbolScopes.check(exp->value))
    throw std::runtime_error("[TypeChecker] Variable no declarada: '" +
                             exp->value + "' usada en la función '" +
                             currentFunction + "'");
  return 0;
}

int TypeCheckerVisitor::visit(AssignmentStatement *stm) {
  const auto target = inferLvalue(stm->target.get());
  if (target.isArray())
    throw std::runtime_error(
        "[TypeChecker] No se puede asignar directamente a un arreglo");
  requireAssignableExpression(target, stm->expression.get(), "asignación");
  return 0;
}

int TypeCheckerVisitor::visit(CompoundAssignmentStatement *stm) {
  const auto target = inferLvalue(stm->target.get());
  if (target.isArray())
    throw std::runtime_error(
        "[TypeChecker] No se puede asignar directamente a un arreglo");
  if (target.isStructObject())
    throw std::runtime_error(
        "[TypeChecker] Asignación compuesta no acepta estructuras");

  const auto lhs = target.decayed();
  const auto rhs = infer(stm->expression.get()).decayed();
  TypeInfo result;

  if (stm->op == PLUS_OP) {
    const auto semantics = analyzeBinary(stm->op, lhs, rhs);
    if (!semantics.integerArithmetic &&
        !(semantics.pointerArithmetic && isPointerType(lhs)))
      throw std::runtime_error(
          "[TypeChecker] Asignación '+=' requiere enteros o puntero más "
          "entero");
    result = semantics.result;
  } else if (stm->op == MINUS_OP) {
    const auto semantics = analyzeBinary(stm->op, lhs, rhs);
    if (!semantics.integerArithmetic &&
        !(semantics.pointerArithmetic && isPointerType(lhs)))
      throw std::runtime_error(
          "[TypeChecker] Asignación '-=' requiere enteros o puntero menos "
          "entero");
    result = semantics.result;
  } else {
    if (!isScalarInteger(lhs) || !isScalarInteger(rhs))
      throw std::runtime_error(
          "[TypeChecker] Asignación compuesta requiere operandos enteros");
    result = analyzeBinary(stm->op, lhs, rhs).result;
  }

  requireAssignable(target, result, "asignación compuesta");
  return 0;
}

int TypeCheckerVisitor::visit(FunctionCallExpression *fcall) {
  if (functionParameterTypes.contains(fcall->name)) {
    requireUserFunctionCall(fcall);
    return 0;
  }

  throw std::runtime_error("[TypeChecker] Función no definida: '" +
                           fcall->name + "' llamada en '" + currentFunction +
                           "'");
}

int TypeCheckerVisitor::visit(IfStatement *stm) {
  requireCondition(stm->condition.get(), "if");

  const std::size_t base = localBytes;

  localBytes = 0;
  stm->then->accept(this);
  std::size_t maxLocales = localBytes;

  if (stm->elseBody) {
    localBytes = 0;
    stm->elseBody->accept(this);
    maxLocales = std::max(maxLocales, localBytes);
  }

  localBytes = base + maxLocales;
  return 0;
}

int TypeCheckerVisitor::visit(ForStatement *stm) {
  symbolScopes.add_level();
  variableTypes.add_level();

  std::visit(
      [&](auto &initializer) {
        using Initializer = std::decay_t<decltype(initializer)>;
        if constexpr (!std::is_same_v<Initializer, std::monostate>)
          if (initializer)
            initializer->accept(this);
      },
      stm->initializer);

  if (stm->condition)
    requireCondition(stm->condition.get(), "for");

  breakableDepth++;
  loopDepth++;
  stm->body->accept(this);
  if (stm->update)
    stm->update->accept(this);
  loopDepth--;
  breakableDepth--;

  variableTypes.remove_level();
  symbolScopes.remove_level();
  return 0;
}

int TypeCheckerVisitor::visit(WhileStatement *stm) {
  requireCondition(stm->condition.get(), "while");
  breakableDepth++;
  loopDepth++;
  stm->body->accept(this);
  loopDepth--;
  breakableDepth--;
  return 0;
}

int TypeCheckerVisitor::visit(ExpressionStatement *stm) {
  if (auto *call =
          dynamic_cast<FunctionCallExpression *>(stm->expression.get())) {
    call->accept(this);
    if (functionReturnTypes.contains(call->name) &&
        functionReturnTypes.at(call->name).isStructObject())
      throw std::runtime_error(
          "[TypeChecker] Llamada con retorno struct requiere destino de "
          "asignación");
    return 0;
  }
  infer(stm->expression.get());
  return 0;
}

int TypeCheckerVisitor::visit(ReturnStatement *r) {
  if (!r->expression) {
    if (!currentReturnType.isVoidObject())
      throw std::runtime_error("[TypeChecker] return sin valor en función '" +
                               currentFunction + "' no void");
    return 0;
  }
  if (currentReturnType.isVoidObject())
    throw std::runtime_error(
        "[TypeChecker] return con valor en función void '" + currentFunction +
        "'");
  if (currentReturnType.isStructObject() &&
      !isLvalueExpression(r->expression.get()))
    throw std::runtime_error(
        "[TypeChecker] return struct requiere objeto direccionable");
  requireAssignableExpression(currentReturnType, r->expression.get(),
                              "return de función '" + currentFunction + "'");
  return 0;
}

int TypeCheckerVisitor::visit(BinaryExpression *exp) {
  infer(exp);
  return 0;
}

int TypeCheckerVisitor::visit(UnaryExpression *exp) {
  infer(exp);
  return 0;
}

int TypeCheckerVisitor::visit(IncDecExpression *exp) {
  infer(exp);
  return 0;
}

int TypeCheckerVisitor::visit(DereferenceExpression *exp) {
  infer(exp);
  return 0;
}

int TypeCheckerVisitor::visit(AddressExpression *exp) {
  infer(exp);
  return 0;
}

int TypeCheckerVisitor::visit(SubscriptExpression *exp) {
  infer(exp);
  return 0;
}

int TypeCheckerVisitor::visit(SizeofExpression *exp) {
  infer(exp);
  return 0;
}

int TypeCheckerVisitor::visit(FieldExpression *exp) {
  inferLvalue(exp);
  return 0;
}

int TypeCheckerVisitor::visit(NumberExpression *) { return 0; }
int TypeCheckerVisitor::visit(StringExpression *) { return 0; }
int TypeCheckerVisitor::visit(Program *) { return 0; }

int TypeCheckerVisitor::visit(DoWhileStatement *stm) {
  requireCondition(stm->condition.get(), "do while");
  breakableDepth++;
  loopDepth++;
  stm->body->accept(this);
  loopDepth--;
  breakableDepth--;
  return 0;
}

int TypeCheckerVisitor::visit(BreakStatement *) {
  if (breakableDepth == 0)
    throw std::runtime_error(
        "[TypeChecker] break fuera de while, do while o switch");
  return 0;
}

int TypeCheckerVisitor::visit(ContinueStatement *) {
  if (loopDepth == 0)
    throw std::runtime_error("[TypeChecker] continue fuera de ciclo");
  return 0;
}

int TypeCheckerVisitor::visit(SwitchStatement *stm) {
  requireInteger(stm->expression.get(), "switch");
  breakableDepth++;
  std::unordered_map<std::int64_t, bool> caseValues;
  for (auto &c : stm->cases) {
    c->expression->accept(this);
    c->value = evaluateConstantInt(c->expression.get(), &structInfos);
    if (caseValues.contains(c->value))
      throw std::runtime_error("[TypeChecker] case duplicado en switch");
    caseValues[c->value] = true;
    for (auto &s : c->body)
      s->accept(this);
  }
  for (auto &s : stm->defaultBody)
    s->accept(this);
  breakableDepth--;
  return 0;
}
