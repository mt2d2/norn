#ifndef IR_H
#define IR_H

class IR {
public:
  enum class Opcode {
    LitInt,
    LoadInt,
    StoreInt,
    EqInt,
    NeqInt,
    LeInt,
    LeqInt,
    GeInt,
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
  IR *getRef1() const;
  IR *getRef2() const;
  void setRef1(IR *ref);
  void setRef2(IR *ref);

  bool yieldsConstant() const;
  bool isJump() const;
  bool isLoad() const;
  bool isStore() const;
  bool hasSideEffect() const;
  bool isPhi() const;

  void clear();

private:
  static unsigned variableNameGen;
  explicit IR(const Opcode op, IR *ref1, IR *ref2, bool constant,
              int64_t intArg);

  std::vector<IR *> references;

  friend std::ostream &operator<<(std::ostream &stream, const IR &ir);
};

std::ostream &operator<<(std::ostream &stream, const IR::Opcode op);
std::ostream &operator<<(std::ostream &stream, const IR &ir);

#endif
