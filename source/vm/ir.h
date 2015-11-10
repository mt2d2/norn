#ifndef IR_H
#define IR_H

struct IR {
  static unsigned int variableNameGen;

  enum class Opcode {
    LitInt,
    LoadInt,
    StoreInt,
    EqInt,
    LeInt,
    LeqInt,
    AddInt,
    SubInt,
    MulInt,
    DivInt,
    ModInt,
    Fjmp,
    Tjmp,
    Ujmp,
    Loop,
    Nop,
    Phi
  };

  Opcode op;
  IR *ref1, *ref2;
  int64_t intArg;
  bool hasConstantArg1;

  std::size_t variableName;
  bool deadCode;

  explicit IR(const Opcode op);
  explicit IR(const Opcode op, const int64_t arg);
  explicit IR(const Opcode op, IR *ref1);
  explicit IR(const Opcode op, IR *ref1, int64_t arg);
  explicit IR(const Opcode op, IR *ref1, IR *ref2);

  bool hasRef1() const;
  bool hasRef2() const;

  bool yieldsConstant() const;
  bool isJump() const;
  bool isLoad() const;
  bool isStore() const;
  bool hasSideEffect() const;
  bool isPhi() const;

  void clear();
};

std::ostream &operator<<(std::ostream &stream, const IR::Opcode op);
std::ostream &operator<<(std::ostream &stream, const IR &ir);

#endif
