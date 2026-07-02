#ifndef ASM_H
#define ASM_H

#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

enum class AsmWidth { None, Byte, Word, Long, Quad };

enum class AsmRegister { RAX, RCX, RDX, RDI, RSI, R8, R9, R10, R11, RBP, RSP };

enum class AsmOperandKind { Register, Immediate, Memory, Label };

struct AsmOperand {
  AsmOperandKind kind = AsmOperandKind::Immediate;
  AsmRegister reg = AsmRegister::RAX;
  AsmRegister base = AsmRegister::RAX;
  AsmRegister index = AsmRegister::RAX;
  AsmWidth width = AsmWidth::Quad;
  long immediate = 0;
  long displacement = 0;
  int scale = 1;
  bool hasBase = false;
  bool hasIndex = false;
  bool ripRelative = false;
  std::string text;
  std::string symbol;

  static AsmOperand regOp(AsmRegister reg, AsmWidth width = AsmWidth::Quad);
  static AsmOperand regText(std::string reg);
  static AsmOperand imm(long value);
  static AsmOperand mem(std::string address);
  static AsmOperand stack(long displacement,
                          AsmRegister base = AsmRegister::RBP);
  static AsmOperand global(std::string symbol);
  static AsmOperand indirect(AsmRegister base, long displacement = 0);
  static AsmOperand indexed(AsmRegister base, AsmRegister index, int scale,
                            long displacement = 0);
  static AsmOperand label(std::string name);
};

enum class AsmItemKind { Instruction, Label, Directive, Blank };

struct AsmInstruction {
  AsmItemKind kind = AsmItemKind::Instruction;
  std::string opcode;
  AsmWidth width = AsmWidth::None;
  std::vector<AsmOperand> operands;
  std::string text;
};

class AsmBuilder {
private:
  std::vector<AsmInstruction> items;

public:
  std::vector<AsmInstruction> &programItems();
  const std::vector<AsmInstruction> &programItems() const;
  void blank();
  void directive(std::string text);
  void label(std::string name);
  void instr(std::string opcode, AsmWidth width = AsmWidth::None,
             std::vector<AsmOperand> operands = {});
  void mov(AsmWidth width, AsmOperand src, AsmOperand dst);
  void movsx(AsmWidth from, AsmWidth to, AsmOperand src, AsmOperand dst);
  void movzx(AsmWidth from, AsmWidth to, AsmOperand src, AsmOperand dst);
  void lea(AsmOperand src, AsmOperand dst);
  void add(AsmWidth width, AsmOperand src, AsmOperand dst);
  void sub(AsmWidth width, AsmOperand src, AsmOperand dst);
  void imul(AsmWidth width, AsmOperand src, AsmOperand dst);
  void div(AsmWidth width, AsmOperand operand);
  void idiv(AsmWidth width, AsmOperand operand);
  void cmp(AsmWidth width, AsmOperand src, AsmOperand dst);
  void push(AsmOperand operand);
  void pop(AsmOperand operand);
  void call(std::string target);
  void jmp(std::string target);
  void je(std::string target);
  void jne(std::string target);
  void set(std::string condition, AsmOperand dst);
  void cqto();
  void ret();
  void write(std::ostream &out) const;
};

std::string asmRegName(AsmRegister reg, AsmWidth width);
std::string asmWidthSuffix(AsmWidth width);
std::string asmOperandText(const AsmOperand &operand);

#endif
