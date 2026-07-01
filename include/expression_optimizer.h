#ifndef EXPRESSION_OPTIMIZER_H
#define EXPRESSION_OPTIMIZER_H

#include "ast.h"
#include "visitor.h"

void optimizeExpressions(Program *program, const SemanticInfo &semantics);

#endif
