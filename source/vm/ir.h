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
  std::size_t ref1, ref2;
  int64_t intArg;

  bool hasConstantArg1;
  bool hasRef1;
  bool hasRef2;

  std::size_t variableName;

  IR(const Opcode op, const int64_t arg);
  IR(const Opcode op, const std::size_t ref1);
  IR(const Opcode op, const std::size_t ref1, const std::size_t ref2);

  bool yieldsConstant() const;
  bool isJump() const;
  bool hasSideEffect() const;
};

std::ostream &operator<<(std::ostream &stream, const IR::Opcode op);
std::ostream &operator<<(std::ostream &stream, const IR &ir);

#endif
