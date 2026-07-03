#include "ast_printer.h"

#include <string>

void AstPrinter::printIndent() {
  for (int i = 0; i < indent; ++i)
    out << "  ";
}

void AstPrinter::println(const std::string &text) {
  printIndent();
  out << text << "\n";
}

std::string AstPrinter::result() const { return out.str(); }

static std::string binaryOpName(BinaryOp op) {
  switch (op) {
  case PLUS_OP: return "+";
  case MINUS_OP: return "-";
  case MUL_OP: return "*";
  case DIV_OP: return "/";
  case MOD_OP: return "%";
  case LE_OP: return "<";
  case GT_OP: return ">";
  case LEQ_OP: return "<=";
  case GEQ_OP: return ">=";
  case EQ_OP: return "==";
  case NE_OP: return "!=";
  case AND_OP: return "&&";
  case OR_OP: return "||";
  }
  return "?";
}

static std::string unaryOpName(UnaryOp op) {
  switch (op) {
  case NOT_OP: return "!";
  case NEG_OP: return "-";
  }
  return "?";
}

int AstPrinter::visit(Program *p) {
  println("Program");
  ++indent;
  for (auto &sd : p->structDeclarations)
    sd->accept(this);
  for (auto &gd : p->globalDeclarations)
    gd->accept(this);
  for (auto &fd : p->functionDeclarations)
    fd->accept(this);
  --indent;
  return 0;
}

int AstPrinter::visit(StructDeclaration *sd) {
  println("StructDeclaration: " + sd->name);
  ++indent;
  for (auto &f : sd->fields)
    f->accept(this);
  --indent;
  return 0;
}

int AstPrinter::visit(FunctionDeclaration *fd) {
  std::string text = "FunctionDeclaration: " + fd->name +
                     " -> " + fd->returnBaseType;
  for (std::size_t i = 0; i < fd->returnPointerDepth; ++i)
    text += "*";
  println(text);
  ++indent;
  for (std::size_t i = 0; i < fd->parameterNames.size(); ++i) {
    std::string param = "Param: " + fd->parameterNames[i] + " : " +
                        fd->parameterBaseTypes[i];
    for (std::size_t j = 0; j < fd->parameterPointerDepths[i]; ++j)
      param += "*";
    println(param);
  }
  if (fd->body)
    fd->body->accept(this);
  --indent;
  return 0;
}

int AstPrinter::visit(VariableDeclaration *vd) {
  std::string text = "VarDecl: " + vd->baseType;
  println(text);
  ++indent;
  for (auto &d : vd->declarators) {
    std::string dtext = d.name;
    for (std::size_t i = 0; i < d.pointerDepth; ++i)
      dtext = "*" + dtext;
    if (d.initializer) {
      dtext += " =";
      println(dtext);
      ++indent;
      d.initializer->accept(this);
      --indent;
    } else {
      println(dtext);
    }
  }
  --indent;
  return 0;
}

int AstPrinter::visit(Body *body) {
  println("Body");
  ++indent;
  for (auto &item : body->items) {
    if (auto *di = dynamic_cast<DeclarationItem *>(item.get()))
      di->declaration->accept(this);
    else if (auto *si = dynamic_cast<StatementItem *>(item.get()))
      si->statement->accept(this);
  }
  --indent;
  return 0;
}

int AstPrinter::visit(ReturnStatement *r) {
  println("ReturnStatement");
  if (r->expression) {
    ++indent;
    r->expression->accept(this);
    --indent;
  }
  return 0;
}

int AstPrinter::visit(ExpressionStatement *stm) {
  println("ExpressionStatement");
  if (stm->expression) {
    ++indent;
    stm->expression->accept(this);
    --indent;
  }
  return 0;
}

int AstPrinter::visit(IfStatement *stm) {
  println("IfStatement");
  ++indent;
  println("condition:");
  ++indent;
  stm->condition->accept(this);
  --indent;
  println("then:");
  stm->then->accept(this);
  if (stm->elseBody) {
    println("else:");
    stm->elseBody->accept(this);
  }
  --indent;
  return 0;
}

int AstPrinter::visit(WhileStatement *stm) {
  println("WhileStatement");
  ++indent;
  println("condition:");
  ++indent;
  stm->condition->accept(this);
  --indent;
  stm->body->accept(this);
  --indent;
  return 0;
}

int AstPrinter::visit(DoWhileStatement *stm) {
  println("DoWhileStatement");
  ++indent;
  stm->body->accept(this);
  println("condition:");
  ++indent;
  stm->condition->accept(this);
  --indent;
  --indent;
  return 0;
}

int AstPrinter::visit(ForStatement *stm) {
  println("ForStatement");
  ++indent;
  println("init:");
  ++indent;
  if (std::holds_alternative<std::unique_ptr<VariableDeclaration>>(
          stm->initializer))
    (*std::get_if<std::unique_ptr<VariableDeclaration>>(&stm->initializer))
        ->accept(this);
  else if (std::holds_alternative<std::unique_ptr<Statement>>(
               stm->initializer))
    (*std::get_if<std::unique_ptr<Statement>>(&stm->initializer))->accept(this);
  --indent;
  if (stm->condition) {
    println("condition:");
    ++indent;
    stm->condition->accept(this);
    --indent;
  }
  if (stm->update) {
    println("update:");
    ++indent;
    stm->update->accept(this);
    --indent;
  }
  stm->body->accept(this);
  --indent;
  return 0;
}

int AstPrinter::visit(BreakStatement *) {
  println("BreakStatement");
  return 0;
}

int AstPrinter::visit(ContinueStatement *) {
  println("ContinueStatement");
  return 0;
}

int AstPrinter::visit(SwitchStatement *stm) {
  println("SwitchStatement");
  ++indent;
  println("expression:");
  ++indent;
  stm->expression->accept(this);
  --indent;
  for (auto &c : stm->cases) {
    println("Case:");
    ++indent;
    c->expression->accept(this);
    for (auto &s : c->body)
      s->accept(this);
    --indent;
  }
  if (!stm->defaultBody.empty()) {
    println("Default:");
    ++indent;
    for (auto &s : stm->defaultBody)
      s->accept(this);
    --indent;
  }
  --indent;
  return 0;
}

int AstPrinter::visit(AssignmentStatement *stm) {
  println("AssignmentStatement");
  ++indent;
  println("target:");
  ++indent;
  stm->target->accept(this);
  --indent;
  println("value:");
  ++indent;
  stm->expression->accept(this);
  --indent;
  --indent;
  return 0;
}

int AstPrinter::visit(CompoundAssignmentStatement *stm) {
  println("CompoundAssignment: " + binaryOpName(stm->op) + "=");
  ++indent;
  println("target:");
  ++indent;
  stm->target->accept(this);
  --indent;
  println("value:");
  ++indent;
  stm->expression->accept(this);
  --indent;
  --indent;
  return 0;
}

int AstPrinter::visit(BinaryExpression *exp) {
  println("BinaryExpression: " + binaryOpName(exp->op));
  ++indent;
  exp->left->accept(this);
  exp->right->accept(this);
  --indent;
  return 0;
}

int AstPrinter::visit(UnaryExpression *exp) {
  println("UnaryExpression: " + unaryOpName(exp->op));
  ++indent;
  exp->operand->accept(this);
  --indent;
  return 0;
}

int AstPrinter::visit(IncDecExpression *exp) {
  std::string prefix = exp->postfix ? "postfix" : "prefix";
  std::string op = exp->op == IncDecOp::Increment ? "++" : "--";
  println("IncDecExpression: " + prefix + " " + op);
  ++indent;
  exp->operand->accept(this);
  --indent;
  return 0;
}

int AstPrinter::visit(NumberExpression *exp) {
  std::string text = "Number: " + std::to_string(exp->value);
  if (exp->typeName)
    text += " [" + *exp->typeName + "]";
  println(text);
  return 0;
}

int AstPrinter::visit(StringExpression *exp) {
  std::string escaped;
  for (char c : exp->value) {
    if (c == '"') escaped += "\\\"";
    else if (c == '\\') escaped += "\\\\";
    else if (c == '\n') escaped += "\\n";
    else if (c == '\t') escaped += "\\t";
    else escaped += c;
  }
  println("String: \"" + escaped + "\"");
  return 0;
}

int AstPrinter::visit(IdentifierExpression *exp) {
  println("Identifier: " + exp->value);
  return 0;
}

int AstPrinter::visit(DereferenceExpression *exp) {
  println("Dereference");
  ++indent;
  exp->operand->accept(this);
  --indent;
  return 0;
}

int AstPrinter::visit(AddressExpression *exp) {
  println("AddressOf");
  ++indent;
  exp->operand->accept(this);
  --indent;
  return 0;
}

int AstPrinter::visit(SubscriptExpression *exp) {
  println("Subscript");
  ++indent;
  println("base:");
  ++indent;
  exp->base->accept(this);
  --indent;
  for (auto &idx : exp->indices) {
    println("index:");
    ++indent;
    idx->accept(this);
    --indent;
  }
  --indent;
  return 0;
}

int AstPrinter::visit(SizeofExpression *exp) {
  if (exp->expression) {
    println("Sizeof");
    ++indent;
    exp->expression->accept(this);
    --indent;
  } else {
    std::string text = "SizeofType: " + exp->baseType;
    for (std::size_t i = 0; i < exp->pointerDepth; ++i)
      text += "*";
    println(text);
  }
  return 0;
}

int AstPrinter::visit(FieldExpression *exp) {
  std::string op = exp->throughPointer ? "->" : ".";
  println("FieldAccess: " + op + exp->field);
  ++indent;
  exp->object->accept(this);
  --indent;
  return 0;
}

int AstPrinter::visit(FunctionCallExpression *fc) {
  println("FunctionCall: " + fc->name);
  ++indent;
  for (auto &arg : fc->arguments) {
    arg->accept(this);
  }
  --indent;
  return 0;
}
