#ifndef TRACE_H
#define TRACE_H

#if !NOJIT

#include <cstdint>
#include <map>
#include <vector>

typedef void (*nativeTraceType)(/*trace exit*/ int64_t *,
                                /*stack adjust*/ int64_t *,
                                /*stack*/ int64_t *,
                                /*memory*/ int64_t *);

struct Instruction;
class Block;

class Trace {
public:
  struct LangLocal {
    unsigned int memPosition;
    unsigned int memOffsetPosition;
  };

  enum State {
    ABORT,
    TRACING,
    COMPLETE,
  };

  // Todo move to its own file set
  struct IR {
    enum class Opcode {
      LitInt,
      LoadInt,
      StoreInt,
      LeInt,
      AddInt,
      MulInt,
      Fjmp,
      Ujmp
    };

    Opcode op;
    std::size_t ref1, ref2;
    int64_t intArg;
    int64_t intArg2;

    bool hasConstantArg;
    bool hasConstantArg2;
    bool hasRef1;
    bool hasRef2;

    std::size_t variableName;

    IR(const Opcode op, const int64_t arg)
        : op(op), ref1(0), ref2(0), intArg(arg), hasConstantArg(true),
          hasConstantArg2(false), hasRef1(false), hasRef2(false),
          variableName(0) {}
    IR(const Opcode op, const std::size_t ref1)
        : op(op), ref1(ref1), ref2(0), intArg(0), hasConstantArg(false),
          hasConstantArg2(false), hasRef1(true), hasRef2(false),
          variableName(0) {}
    IR(const Opcode op, const std::size_t ref1, const std::size_t ref2)
        : op(op), ref1(ref1), ref2(ref2), intArg(0), hasConstantArg(false),
          hasConstantArg2(false), hasRef1(true), hasRef2(true),
          variableName(0) {}

    bool yieldsConstant() const { return op == Opcode::LitInt; }
    bool isJump() const { return op == Opcode::Fjmp || op == Opcode::Ujmp; }
    bool hasSideEffect() const {
      return op == Opcode::StoreInt || op == Opcode::LoadInt || this->isJump();
    }
  };

  Trace();
  ~Trace();
  State record(const Instruction *i);
  bool is_complete() const;
  void debug() const;
  void compile(const bool debug);
  nativeTraceType get_native_ptr() const;
  uint64_t get_trace_exit(int offset) const;
  std::map<const Block *, unsigned int> get_trace_calls() const;

private:
  bool is_head(const Instruction *i) const;
  void convertBytecodeToIR();
  void assignVariableName();
  void propagateConstants();
  void deadCodeElimination();
  void jit(const bool debug);
  void identify_trace_exits();
  void identify_trace_calls();

  State last_state;
  std::vector<const Instruction *> bytecode;
  std::vector<IR> instructions;
  std::vector<uint64_t> traceExits;
  std::map<const Block *, unsigned int> calls;
  nativeTraceType nativePtr;
};

#endif // !NOJIT

#endif // TRACE_H
