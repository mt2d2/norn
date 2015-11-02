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

  IR(const Opcode op, const int64_t arg)
      : op(op), ref1(0), ref2(0), intArg(arg), hasConstantArg1(true),
        hasRef1(false), hasRef2(false), variableName(0) {}
  IR(const Opcode op, const std::size_t ref1)
      : op(op), ref1(ref1), ref2(0), intArg(0), hasConstantArg1(false),
        hasRef1(true), hasRef2(false), variableName(0) {}
  IR(const Opcode op, const std::size_t ref1, const std::size_t ref2)
      : op(op), ref1(ref1), ref2(ref2), intArg(0), hasConstantArg1(false),
        hasRef1(true), hasRef2(true), variableName(0) {}

  bool yieldsConstant() const;
  bool isJump() const;
  bool hasSideEffect() const;
};

std::ostream &operator<<(std::ostream &stream, const IR::Opcode op);
std::ostream &operator<<(std::ostream &stream, const IR &ir);

#endif
