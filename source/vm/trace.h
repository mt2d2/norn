#ifndef TRACE_H
#define TRACE_H

#if !NOJIT

#include <cstdint>
#include <list>
#include <map>
#include <vector>

#include "ir.h"

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
  void propagateConstants();
  void eliminateDeadCode();
  void hoistLoads();
  void sinkStores();
  void jit(const bool debug);
  void identify_trace_exits();

  State last_state;
  std::vector<const Instruction *> bytecode;
  std::list<IR> instructions;
  // [lang_local, {load*, phi*}]
  std::map<int64_t, std::pair<IR *, IR *>> phisFor;
  std::vector<uint64_t> traceExits;
  nativeTraceType nativePtr;
};

#endif // !NOJIT

#endif // TRACE_H
