#ifndef ASM_PEEPHOLE_H
#define ASM_PEEPHOLE_H

#include "asm.h"

#include <vector>

void optimizeAssembly(std::vector<AsmInstruction> &items);

#endif
