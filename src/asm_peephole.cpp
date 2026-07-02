#include "../include/asm_peephole.h"

#include <algorithm>
#include <utility>

namespace {
bool isInstruction(const AsmInstruction &item) {
  return item.kind == AsmItemKind::Instruction;
}

bool isBlank(const AsmInstruction &item) {
  return item.kind == AsmItemKind::Blank;
}

bool isLabel(const AsmInstruction &item) {
  return item.kind == AsmItemKind::Label;
}

bool isDirective(const AsmInstruction &item) {
  return item.kind == AsmItemKind::Directive;
}

bool isTypedRegister(const AsmOperand &operand) {
  return operand.kind == AsmOperandKind::Register && operand.text.empty();
}

bool sameRegister(const AsmOperand &left, const AsmOperand &right) {
  return isTypedRegister(left) && isTypedRegister(right) &&
         left.reg == right.reg && left.width == right.width;
}

bool isMov(const AsmInstruction &item) {
  return isInstruction(item) && item.opcode == "mov" &&
         item.operands.size() == 2;
}

bool isTrueNoOpRegisterMove(const AsmInstruction &item) {
  if (!isMov(item) || !sameRegister(item.operands[0], item.operands[1]))
    return false;
  return item.width != AsmWidth::Long;
}

std::size_t nextNonBlank(const std::vector<AsmInstruction> &items,
                         std::size_t index) {
  while (index < items.size() && isBlank(items[index]))
    ++index;
  return index;
}

bool removeRedundantMoves(std::vector<AsmInstruction> &items) {
  const auto original = items.size();
  items.erase(std::remove_if(items.begin(), items.end(), [](const auto &item) {
                return isTrueNoOpRegisterMove(item);
              }),
              items.end());
  return items.size() != original;
}

bool isJmp(const AsmInstruction &item) {
  return isInstruction(item) && item.opcode == "jmp" &&
         item.operands.size() == 1 &&
         item.operands[0].kind == AsmOperandKind::Label;
}

bool isTerminator(const AsmInstruction &item) {
  return isInstruction(item) && (item.opcode == "jmp" || item.opcode == "ret");
}

bool removeJumpsToNextLabel(std::vector<AsmInstruction> &items) {
  bool changed = false;
  for (std::size_t i = 0; i < items.size();) {
    if (!isJmp(items[i])) {
      ++i;
      continue;
    }

    const std::size_t next = nextNonBlank(items, i + 1);
    if (next < items.size() && isLabel(items[next]) &&
        items[i].operands[0].text == items[next].text) {
      items.erase(items.begin() + static_cast<long>(i));
      changed = true;
      continue;
    }
    ++i;
  }
  return changed;
}

bool removeUnreachableAfterTerminators(std::vector<AsmInstruction> &items) {
  bool changed = false;
  for (std::size_t i = 0; i < items.size();) {
    if (!isTerminator(items[i])) {
      ++i;
      continue;
    }

    std::size_t end = i + 1;
    while (end < items.size() && !isLabel(items[end]) &&
           !isDirective(items[end]))
      ++end;

    if (end > i + 1) {
      items.erase(items.begin() + static_cast<long>(i + 1),
                  items.begin() + static_cast<long>(end));
      changed = true;
    }
    ++i;
  }
  return changed;
}

bool removeDuplicateBlanks(std::vector<AsmInstruction> &items) {
  bool changed = false;
  for (std::size_t i = 1; i < items.size();) {
    if (isBlank(items[i - 1]) && isBlank(items[i])) {
      items.erase(items.begin() + static_cast<long>(i));
      changed = true;
      continue;
    }
    ++i;
  }
  return changed;
}
} // namespace

void optimizeAssembly(std::vector<AsmInstruction> &items) {
  bool changed = false;
  do {
    changed = false;
    changed |= removeRedundantMoves(items);
    changed |= removeJumpsToNextLabel(items);
    changed |= removeUnreachableAfterTerminators(items);
    changed |= removeDuplicateBlanks(items);
  } while (changed);
}
