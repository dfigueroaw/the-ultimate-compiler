#ifndef CONSTANT_FOLDER_H
#define CONSTANT_FOLDER_H

#include "ast.h"
#include "visitor.h"

void foldConstants(Program *program, const SemanticInfo &semantics);

#endif
