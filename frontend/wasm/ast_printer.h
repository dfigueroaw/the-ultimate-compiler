#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#include "ast.h"
#include "visitor.h"

#include <sstream>
#include <string>

class AstPrinter : public Visitor {
private:
  std::ostringstream out;
  int indent = 0;

  void println(const std::string &text);
  void printIndent();

public:
  std::string result() const;

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
  int visit(WhileStatement *stm) override;
  int visit(DoWhileStatement *stm) override;
  int visit(IfStatement *stm) override;
  int visit(AssignmentStatement *stm) override;
  int visit(CompoundAssignmentStatement *stm) override;
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
