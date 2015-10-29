#ifndef TRACE_H
#define TRACE_H

#if !NOJIT

#include <vector>
#include <map>

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
    const IR *ref1, *ref2;
    int64_t intArg;

    IR(const Opcode op, const int64_t arg)
        : op(op), ref1(nullptr), ref2(nullptr), intArg(arg) {}
    IR(const Opcode op, const IR *ref1)
        : op(op), ref1(ref1), ref2(nullptr), intArg(0) {}
    IR(const Opcode op, const IR *ref1, const IR *ref2)
        : op(op), ref1(ref1), ref2(ref2), intArg(0) {}
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
