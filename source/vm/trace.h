#ifndef TRACE_H
#define TRACE_H

#include <vector>
#include <map>
#include <stack>

#include <asmjit/asmjit.h>

typedef void (*nativeTraceType)(int64_t *, int64_t *);

struct Instruction;

struct LangLocal {
  unsigned int memPosition;
  unsigned int memOffsetPosition;
  asmjit::host::GpVar cVar;
};

class Trace {
public:
  Trace();
  ~Trace();
  void record(const Instruction *i);
  bool is_head(const Instruction *i) const;
  size_t root_function_size() const;
  void debug() const;
  void jit(bool debug);
  nativeTraceType get_native_ptr() const;

private:
  void identify_locals(asmjit::host::Compiler &c);
  void load_locals(asmjit::host::Compiler &c, const asmjit::host::GpVar &mp);
  void store_locals(asmjit::host::Compiler &c, const asmjit::host::GpVar &mp);
  std::stack<asmjit::host::GpVar>
  identifyValuesToRetrieveFromLangStack(asmjit::host::Compiler &c);

  std::map<int64_t, LangLocal> locals;
  std::vector<const Instruction *> instructions;
  asmjit::JitRuntime runtime;
  nativeTraceType nativePtr;
  size_t rootFunctionSize;
  size_t callDepth;
};

#endif // TRACE_H
