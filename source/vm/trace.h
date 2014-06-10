#ifndef TRACE_H
#define TRACE_H

#include <vector>
#include <map>
#include <stack>

#include <asmjit/asmjit.h>

typedef void (*nativeTraceType)(int64_t *, int64_t *);

struct Instruction;

class Trace {
public:
  Trace();
  ~Trace();
  void record(const Instruction *i);
  bool is_head(const Instruction *i) const;
  void debug() const;
  void jit();
  nativeTraceType get_native_ptr() const;

private:
  void load_locals(asmjit::host::Compiler &c, const asmjit::host::GpVar &mp);
  std::stack<asmjit::host::GpVar>
  identifyValuesToRetrieveFromLangStack(asmjit::host::Compiler &c);

  std::map<int64_t, asmjit::host::GpVar> locals;
  std::vector<const Instruction *> instructions;
  asmjit::JitRuntime runtime;
  nativeTraceType nativePtr;
};

#endif // TRACE_H
