#ifndef TRACE_H
#define TRACE_H

#include <vector>
#include <map>
#include <stack>

#include <asmjit/asmjit.h>

typedef void (*nativeTraceType)(int64_t *, int64_t *, int64_t *, int64_t *);

struct Instruction;
class Block;

struct LangLocal {
  unsigned int memPosition;
  unsigned int memOffsetPosition;
  asmjit::host::GpVar cVar;
};

class Trace {
public:
  Trace() {}
  Trace(asmjit::JitRuntime *runtime);
  ~Trace();
  void record(const Instruction *i);
  bool is_head(const Instruction *i) const;
  void debug() const;
  void compile(const bool debug);
  nativeTraceType get_native_ptr() const;
  std::vector<uint64_t> get_trace_exits() const;
  std::vector<const Block *> get_trace_calls() const;

private:
  void jit(const bool debug);
  void identify_trace_exits();
  void identify_trace_calls();
  std::map<int64_t, asmjit::host::GpVar>
  identify_literals(asmjit::host::Compiler &c);
  void load_literals(const std::map<int64_t, asmjit::host::GpVar> &literals,
                     asmjit::host::Compiler &c);
  std::map<int64_t, LangLocal> identify_locals(asmjit::host::Compiler &c);
  void load_locals(const std::map<int64_t, LangLocal> &locals,
                   asmjit::host::Compiler &c, const asmjit::host::GpVar &mp);
  void restore_locals(const std::map<int64_t, LangLocal> &locals,
                      asmjit::host::Compiler &c, const asmjit::host::GpVar &mp);
  void restore_stack(
      const std::map<const Instruction *, asmjit::host::GpVar> &stackMap,
      asmjit::host::Compiler &c, const asmjit::host::GpVar &traceExitPtr,
      const asmjit::host::GpVar &stack, const asmjit::host::GpVar &stackAdjust);

  asmjit::JitRuntime *runtime;
  std::vector<const Instruction *> instructions;
  std::vector<uint64_t> traceExits;
  std::vector<const Block *> calls;
  nativeTraceType nativePtr;
};

#endif // TRACE_H
