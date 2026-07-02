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

bool referencesRsp(const AsmOperand &operand) {
  if (isTypedRegister(operand))
    return operand.reg == AsmRegister::RSP;
  if (operand.kind != AsmOperandKind::Memory || !operand.text.empty())
    return false;
  return (operand.hasBase && operand.base == AsmRegister::RSP) ||
         (operand.hasIndex && operand.index == AsmRegister::RSP);
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

bool isPush(const AsmInstruction &item) {
  return isInstruction(item) && item.opcode == "push" &&
         item.operands.size() == 1;
}

bool isPop(const AsmInstruction &item) {
  return isInstruction(item) && item.opcode == "pop" &&
         item.operands.size() == 1;
}

bool isJmp(const AsmInstruction &item) {
  return isInstruction(item) && item.opcode == "jmp" &&
         item.operands.size() == 1 &&
         item.operands[0].kind == AsmOperandKind::Label;
}

bool isTerminator(const AsmInstruction &item) {
  return isInstruction(item) && (item.opcode == "jmp" || item.opcode == "ret");
}

bool canRewritePushPopToMov(const AsmInstruction &push,
                            const AsmInstruction &pop) {
  const auto &src = push.operands[0];
  const auto &dst = pop.operands[0];
  if (!isTypedRegister(src))
    return false;
  if (!isTypedRegister(dst))
    return false;
  if (referencesRsp(src) || referencesRsp(dst))
    return false;
  return true;
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

bool simplifyPushPop(std::vector<AsmInstruction> &items) {
  bool changed = false;
  for (std::size_t i = 0; i + 1 < items.size();) {
    if (!isPush(items[i]) || !isPop(items[i + 1])) {
      ++i;
      continue;
    }

    if (sameRegister(items[i].operands[0], items[i + 1].operands[0]) &&
        !referencesRsp(items[i].operands[0])) {
      items.erase(items.begin() + static_cast<long>(i),
                  items.begin() + static_cast<long>(i + 2));
      changed = true;
      continue;
    }

    if (canRewritePushPopToMov(items[i], items[i + 1])) {
      AsmInstruction replacement;
      replacement.kind = AsmItemKind::Instruction;
      replacement.opcode = "mov";
      replacement.width = AsmWidth::Quad;
      replacement.operands = {items[i].operands[0], items[i + 1].operands[0]};
      items[i] = std::move(replacement);
      items.erase(items.begin() + static_cast<long>(i + 1));
      changed = true;
      ++i;
      continue;
    }

    ++i;
  }
  return changed;
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
    changed |= simplifyPushPop(items);
    changed |= removeJumpsToNextLabel(items);
    changed |= removeUnreachableAfterTerminators(items);
    changed |= removeDuplicateBlanks(items);
  } while (changed);
}
