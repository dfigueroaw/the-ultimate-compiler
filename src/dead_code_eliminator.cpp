#include "../include/dead_code_eliminator.h"

#include <cstdint>
#include <list>
#include <memory>
#include <optional>
#include <type_traits>
#include <variant>

namespace {
class DeadCodeEliminator {
public:
  void eliminate(Program *program) {
    for (auto &function : program->functionDeclarations)
      if (function->hasBody())
        eliminateBody(function->body.get());
  }

private:
  void eliminateBody(Body *body) {
    for (auto it = body->items.begin(); it != body->items.end();) {
      if (auto *statementItem = dynamic_cast<StatementItem *>(it->get())) {
        eliminateStatement(statementItem->statement);
        if (!statementItem->statement) {
          it = body->items.erase(it);
          continue;
        }
        if (isTerminator(statementItem->statement.get())) {
          ++it;
          body->items.erase(it, body->items.end());
          return;
        }
      }
      ++it;
    }
  }

  void eliminateStatementList(std::list<std::unique_ptr<Statement>> &list) {
    for (auto it = list.begin(); it != list.end();) {
      eliminateStatement(*it);
      if (!*it) {
        it = list.erase(it);
        continue;
      }
      if (isTerminator(it->get())) {
        ++it;
        list.erase(it, list.end());
        return;
      }
      ++it;
    }
  }

  void eliminateStatement(std::unique_ptr<Statement> &statement) {
    if (!statement)
      return;

    if (auto *ifStm = dynamic_cast<IfStatement *>(statement.get())) {
      eliminateIf(statement, ifStm);
    } else if (auto *forStm = dynamic_cast<ForStatement *>(statement.get())) {
      eliminateFor(statement, forStm);
    } else if (auto *whileStm = dynamic_cast<WhileStatement *>(statement.get())) {
      eliminateWhile(statement, whileStm);
    } else if (auto *doWhile =
                   dynamic_cast<DoWhileStatement *>(statement.get())) {
      eliminateBody(doWhile->body.get());
    } else if (auto *switchStm =
                   dynamic_cast<SwitchStatement *>(statement.get())) {
      for (auto &caseStm : switchStm->cases)
        eliminateStatementList(caseStm->body);
      eliminateStatementList(switchStm->defaultBody);
    }
  }

  void eliminateIf(std::unique_ptr<Statement> &statement, IfStatement *ifStm) {
    const auto condition = literalValue(ifStm->condition.get());
    if (!condition) {
      eliminateBody(ifStm->then.get());
      if (ifStm->elseBody)
        eliminateBody(ifStm->elseBody.get());
      return;
    }

    if (*condition == 0) {
      if (ifStm->elseBody) {
        ifStm->then->items.clear();
        eliminateBody(ifStm->elseBody.get());
      } else {
        statement.reset();
      }
      return;
    }

    eliminateBody(ifStm->then.get());
    ifStm->elseBody.reset();
  }

  void eliminateFor(std::unique_ptr<Statement> &statement, ForStatement *forStm) {
    std::visit(
        [&](auto &initializer) {
          using Initializer = std::decay_t<decltype(initializer)>;
          if constexpr (std::is_same_v<Initializer,
                                       std::unique_ptr<VariableDeclaration>>) {
            if (initializer)
              eliminateDeclaration(initializer.get());
          } else if constexpr (std::is_same_v<Initializer,
                                              std::unique_ptr<Statement>>) {
            if (initializer)
              eliminateStatement(initializer);
          }
        },
        forStm->initializer);

    if (forStm->update)
      eliminateStatement(forStm->update);

    if (initializerEmpty(forStm) && isLiteral(forStm->condition.get(), 0)) {
      statement.reset();
      return;
    }

    eliminateBody(forStm->body.get());
  }

  void eliminateWhile(std::unique_ptr<Statement> &statement,
                      WhileStatement *whileStm) {
    if (isLiteral(whileStm->condition.get(), 0)) {
      statement.reset();
      return;
    }

    eliminateBody(whileStm->body.get());
  }

  void eliminateDeclaration(VariableDeclaration *declaration) {
    (void)declaration;
  }

  bool initializerEmpty(const ForStatement *forStm) const {
    return std::holds_alternative<std::monostate>(forStm->initializer);
  }

  bool isTerminator(const Statement *statement) const {
    return dynamic_cast<const ReturnStatement *>(statement) ||
           dynamic_cast<const BreakStatement *>(statement) ||
           dynamic_cast<const ContinueStatement *>(statement);
  }

  bool isLiteral(Expression *expression, std::int64_t value) const {
    const auto literal = literalValue(expression);
    return literal && *literal == value;
  }

  std::optional<std::int64_t> literalValue(Expression *expression) const {
    auto *number = dynamic_cast<NumberExpression *>(expression);
    if (!number)
      return std::nullopt;
    return number->value;
  }
};
} // namespace

void eliminateDeadCode(Program *program) {
  DeadCodeEliminator eliminator;
  eliminator.eliminate(program);
}
