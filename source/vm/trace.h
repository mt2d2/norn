#ifndef TRACE_H
#define TRACE_H

#include <vector>
#include <map>
#include <stack>

#include <asmjit/asmjit.h>

typedef void (*nativeTraceType)(unsigned int *, int64_t *, int64_t *);

struct Instruction;

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
  size_t root_function_size() const;
  void debug() const;
  void compile(const bool debug);
  nativeTraceType get_native_ptr() const;
  std::vector<unsigned int> get_trace_exits() const;

private:
  void jit(const bool debug);
  void identify_trace_exits();
  std::map<int64_t, asmjit::host::GpVar>
  identify_literals(asmjit::host::Compiler &c);
  void load_literals(const std::map<int64_t, asmjit::host::GpVar> &literals,
                     asmjit::host::Compiler &c);
  std::map<int64_t, LangLocal> identify_locals(asmjit::host::Compiler &c);
  void load_locals(const std::map<int64_t, LangLocal> &locals,
                   asmjit::host::Compiler &c, const asmjit::host::GpVar &mp);
  void store_locals(const std::map<int64_t, LangLocal> &locals,
                    asmjit::host::Compiler &c, const asmjit::host::GpVar &mp);

  asmjit::JitRuntime *runtime;
  std::vector<const Instruction *> instructions;
  std::vector<unsigned int> traceExits;
  nativeTraceType nativePtr;
  size_t rootFunctionSize;
  size_t callDepth;
};

#endif // TRACE_H
