#include "../include/asm.h"

#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>

AsmOperand AsmOperand::regOp(AsmRegister r, AsmWidth w) {
  AsmOperand operand;
  operand.kind = AsmOperandKind::Register;
  operand.reg = r;
  operand.width = w;
  return operand;
}

AsmOperand AsmOperand::regText(std::string r) {
  AsmOperand operand;
  operand.kind = AsmOperandKind::Register;
  operand.text = std::move(r);
  return operand;
}

AsmOperand AsmOperand::imm(long value) {
  AsmOperand operand;
  operand.kind = AsmOperandKind::Immediate;
  operand.immediate = value;
  return operand;
}

AsmOperand AsmOperand::mem(std::string address) {
  AsmOperand operand;
  operand.kind = AsmOperandKind::Memory;
  operand.text = std::move(address);
  return operand;
}

AsmOperand AsmOperand::stack(long offset, AsmRegister baseReg) {
  AsmOperand operand;
  operand.kind = AsmOperandKind::Memory;
  operand.base = baseReg;
  operand.displacement = offset;
  operand.hasBase = true;
  return operand;
}

AsmOperand AsmOperand::global(std::string name) {
  AsmOperand operand;
  operand.kind = AsmOperandKind::Memory;
  operand.symbol = std::move(name);
  operand.ripRelative = true;
  return operand;
}

AsmOperand AsmOperand::indirect(AsmRegister baseReg, long offset) {
  AsmOperand operand;
  operand.kind = AsmOperandKind::Memory;
  operand.base = baseReg;
  operand.displacement = offset;
  operand.hasBase = true;
  return operand;
}

AsmOperand AsmOperand::indexed(AsmRegister baseReg, AsmRegister indexReg,
                               int scaleValue, long offset) {
  AsmOperand operand;
  operand.kind = AsmOperandKind::Memory;
  operand.base = baseReg;
  operand.index = indexReg;
  operand.scale = scaleValue;
  operand.displacement = offset;
  operand.hasBase = true;
  operand.hasIndex = true;
  return operand;
}

AsmOperand AsmOperand::label(std::string name) {
  AsmOperand operand;
  operand.kind = AsmOperandKind::Label;
  operand.text = std::move(name);
  return operand;
}

std::string asmRegName(AsmRegister reg, AsmWidth width) {
  switch (reg) {
  case AsmRegister::RAX:
    if (width == AsmWidth::Byte)
      return "%al";
    if (width == AsmWidth::Word)
      return "%ax";
    if (width == AsmWidth::Long)
      return "%eax";
    return "%rax";
  case AsmRegister::RCX:
    if (width == AsmWidth::Byte)
      return "%cl";
    if (width == AsmWidth::Word)
      return "%cx";
    if (width == AsmWidth::Long)
      return "%ecx";
    return "%rcx";
  case AsmRegister::RDX:
    if (width == AsmWidth::Byte)
      return "%dl";
    if (width == AsmWidth::Word)
      return "%dx";
    if (width == AsmWidth::Long)
      return "%edx";
    return "%rdx";
  case AsmRegister::RDI:
    if (width == AsmWidth::Byte)
      return "%dil";
    if (width == AsmWidth::Word)
      return "%di";
    if (width == AsmWidth::Long)
      return "%edi";
    return "%rdi";
  case AsmRegister::RSI:
    if (width == AsmWidth::Byte)
      return "%sil";
    if (width == AsmWidth::Word)
      return "%si";
    if (width == AsmWidth::Long)
      return "%esi";
    return "%rsi";
  case AsmRegister::R8:
    if (width == AsmWidth::Byte)
      return "%r8b";
    if (width == AsmWidth::Word)
      return "%r8w";
    if (width == AsmWidth::Long)
      return "%r8d";
    return "%r8";
  case AsmRegister::R9:
    if (width == AsmWidth::Byte)
      return "%r9b";
    if (width == AsmWidth::Word)
      return "%r9w";
    if (width == AsmWidth::Long)
      return "%r9d";
    return "%r9";
  case AsmRegister::R10:
    if (width == AsmWidth::Byte)
      return "%r10b";
    if (width == AsmWidth::Word)
      return "%r10w";
    if (width == AsmWidth::Long)
      return "%r10d";
    return "%r10";
  case AsmRegister::R11:
    if (width == AsmWidth::Byte)
      return "%r11b";
    if (width == AsmWidth::Word)
      return "%r11w";
    if (width == AsmWidth::Long)
      return "%r11d";
    return "%r11";
  case AsmRegister::RBP:
    if (width == AsmWidth::Byte)
      return "%bpl";
    if (width == AsmWidth::Word)
      return "%bp";
    if (width == AsmWidth::Long)
      return "%ebp";
    return "%rbp";
  case AsmRegister::RSP:
    if (width == AsmWidth::Byte)
      return "%spl";
    if (width == AsmWidth::Word)
      return "%sp";
    if (width == AsmWidth::Long)
      return "%esp";
    return "%rsp";
  }
  throw std::runtime_error("[Asm] Registro no soportado");
}

std::string asmWidthSuffix(AsmWidth width) {
  if (width == AsmWidth::Byte)
    return "b";
  if (width == AsmWidth::Word)
    return "w";
  if (width == AsmWidth::Long)
    return "l";
  if (width == AsmWidth::Quad)
    return "q";
  return "";
}

std::string asmOperandText(const AsmOperand &operand) {
  if (operand.kind == AsmOperandKind::Register) {
    if (!operand.text.empty())
      return operand.text;
    return asmRegName(operand.reg, operand.width);
  }
  if (operand.kind == AsmOperandKind::Immediate)
    return "$" + std::to_string(operand.immediate);
  if (operand.kind == AsmOperandKind::Memory && operand.text.empty()) {
    std::string result;
    if (!operand.symbol.empty())
      result += operand.symbol;
    if (operand.displacement != 0 ||
        (operand.symbol.empty() && !operand.hasBase && !operand.ripRelative))
      result += std::to_string(operand.displacement);
    if (operand.ripRelative)
      result += "(%rip)";
    else if (operand.hasBase || operand.hasIndex) {
      result += "(";
      if (operand.hasBase)
        result += asmRegName(operand.base, AsmWidth::Quad);
      if (operand.hasIndex) {
        result += ", ";
        result += asmRegName(operand.index, AsmWidth::Quad);
        result += ", ";
        result += std::to_string(operand.scale);
      }
      result += ")";
    }
    return result;
  }
  return operand.text;
}

void AsmBuilder::blank() {
  AsmInstruction item;
  item.kind = AsmItemKind::Blank;
  items.push_back(std::move(item));
}

void AsmBuilder::directive(std::string text) {
  AsmInstruction item;
  item.kind = AsmItemKind::Directive;
  item.text = std::move(text);
  items.push_back(std::move(item));
}

void AsmBuilder::label(std::string name) {
  AsmInstruction item;
  item.kind = AsmItemKind::Label;
  item.text = std::move(name);
  items.push_back(std::move(item));
}

void AsmBuilder::instr(std::string opcode, AsmWidth width,
                       std::vector<AsmOperand> operands) {
  AsmInstruction item;
  item.kind = AsmItemKind::Instruction;
  item.opcode = std::move(opcode);
  item.width = width;
  item.operands = std::move(operands);
  items.push_back(std::move(item));
}

void AsmBuilder::mov(AsmWidth width, AsmOperand src, AsmOperand dst) {
  instr("mov", width, {std::move(src), std::move(dst)});
}

void AsmBuilder::movsx(AsmWidth from, AsmWidth to, AsmOperand src,
                       AsmOperand dst) {
  instr("movs" + asmWidthSuffix(from) + asmWidthSuffix(to), AsmWidth::None,
        {std::move(src), std::move(dst)});
}

void AsmBuilder::movzx(AsmWidth from, AsmWidth to, AsmOperand src,
                       AsmOperand dst) {
  instr("movz" + asmWidthSuffix(from) + asmWidthSuffix(to), AsmWidth::None,
        {std::move(src), std::move(dst)});
}

void AsmBuilder::lea(AsmOperand src, AsmOperand dst) {
  instr("lea", AsmWidth::Quad, {std::move(src), std::move(dst)});
}

void AsmBuilder::add(AsmWidth width, AsmOperand src, AsmOperand dst) {
  instr("add", width, {std::move(src), std::move(dst)});
}

void AsmBuilder::sub(AsmWidth width, AsmOperand src, AsmOperand dst) {
  instr("sub", width, {std::move(src), std::move(dst)});
}

void AsmBuilder::imul(AsmWidth width, AsmOperand src, AsmOperand dst) {
  instr("imul", width, {std::move(src), std::move(dst)});
}

void AsmBuilder::div(AsmWidth width, AsmOperand operand) {
  instr("div", width, {std::move(operand)});
}

void AsmBuilder::idiv(AsmWidth width, AsmOperand operand) {
  instr("idiv", width, {std::move(operand)});
}

void AsmBuilder::cmp(AsmWidth width, AsmOperand src, AsmOperand dst) {
  instr("cmp", width, {std::move(src), std::move(dst)});
}

void AsmBuilder::push(AsmOperand operand) {
  instr("push", AsmWidth::Quad, {std::move(operand)});
}

void AsmBuilder::pop(AsmOperand operand) {
  instr("pop", AsmWidth::Quad, {std::move(operand)});
}

void AsmBuilder::call(std::string target) {
  instr("call", AsmWidth::None, {AsmOperand::label(std::move(target))});
}

void AsmBuilder::jmp(std::string target) {
  instr("jmp", AsmWidth::None, {AsmOperand::label(std::move(target))});
}

void AsmBuilder::je(std::string target) {
  instr("je", AsmWidth::None, {AsmOperand::label(std::move(target))});
}

void AsmBuilder::jne(std::string target) {
  instr("jne", AsmWidth::None, {AsmOperand::label(std::move(target))});
}

void AsmBuilder::set(std::string condition, AsmOperand dst) {
  instr("set" + std::move(condition), AsmWidth::None, {std::move(dst)});
}

void AsmBuilder::cqto() { instr("cqto"); }

void AsmBuilder::ret() { instr("ret"); }

void AsmBuilder::write(std::ostream &out) const {
  for (const auto &item : items) {
    if (item.kind == AsmItemKind::Blank) {
      out << "\n";
    } else if (item.kind == AsmItemKind::Directive) {
      out << item.text << "\n";
    } else if (item.kind == AsmItemKind::Label) {
      out << item.text << ":\n";
    } else {
      out << "  " << item.opcode << asmWidthSuffix(item.width);
      if (!item.operands.empty()) {
        out << " ";
        for (std::size_t i = 0; i < item.operands.size(); ++i) {
          if (i > 0)
            out << ", ";
          out << asmOperandText(item.operands[i]);
        }
      }
      out << "\n";
    }
  }
}
