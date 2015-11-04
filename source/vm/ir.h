#ifndef IR_H
#define IR_H

struct IR {
  enum class Opcode {
    LitInt,
    LoadInt,
    StoreInt,
    LeInt,
    AddInt,
    SubInt,
    MulInt,
    DivInt,
    Fjmp,
    Ujmp
  };

  Opcode op;
  IR *ref1, *ref2;
  int64_t intArg;
  bool hasConstantArg1;

  std::size_t variableName;
  bool deadCode;

  IR(const Opcode op, const int64_t arg);
  IR(const Opcode op, IR *ref1);
  IR(const Opcode op, IR *ref1, IR *ref2);

  bool hasRef1() const;
  bool hasRef2() const;

  bool yieldsConstant() const;
  bool isJump() const;
  bool hasSideEffect() const;
};

std::ostream &operator<<(std::ostream &stream, const IR::Opcode op);
std::ostream &operator<<(std::ostream &stream, const IR &ir);

#endif
