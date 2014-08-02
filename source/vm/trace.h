#ifndef TRACE_H
#define TRACE_H

#if !NOJIT

#include <vector>
#include <map>

#include <asmjit/asmjit.h>

typedef void (*nativeTraceType)(int64_t *, int64_t *, int64_t *, int64_t *);

struct Instruction;
class Block;

class Trace {
public:
  struct LangLocal {
    unsigned int memPosition;
    unsigned int memOffsetPosition;
    std::vector<asmjit::GpVar> cVars;
    const asmjit::GpVar &getRoot() { return cVars.at(0); }
    const const asmjit::GpVar &getRecentest() const {
      return cVars.at(cVars.size() - 1);
    }
  };

  enum State {
    ABORT,
    TRACING,
    COMPLETE,
  };

  Trace() {}
  Trace(asmjit::JitRuntime *runtime);
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
  void jit(const bool debug);
  void identify_trace_exits();
  void identify_trace_calls();
  std::map<int64_t, asmjit::GpVar> identify_literals(asmjit::X86Compiler &c);
  void load_literals(const std::map<int64_t, asmjit::GpVar> &literals,
                     asmjit::X86Compiler &c);
  std::map<int64_t, LangLocal> identify_locals(asmjit::X86Compiler &c);
  void load_locals(const std::map<int64_t, LangLocal> &locals,
                   asmjit::X86Compiler &c, const asmjit::GpVar &mp);
  void restore_locals(const std::map<int64_t, LangLocal> &locals,
                      asmjit::X86Compiler &c, const asmjit::GpVar &mp);
  void restore_stack(
      const std::vector<
          std::vector<std::pair<const Instruction *, asmjit::GpVar>>> &stackMap,
      asmjit::X86Compiler &c, const asmjit::GpVar &traceExitPtr,
      const asmjit::GpVar &stack, const asmjit::GpVar &stackAdjust);
  void mergePhis(asmjit::X86Compiler &c,
                 const std::map<int64_t, LangLocal> locals);

  State last_state;
  asmjit::JitRuntime *runtime;
  std::vector<const Instruction *> instructions;
  std::vector<uint64_t> traceExits;
  std::map<const Block *, unsigned int> calls;
  nativeTraceType nativePtr;
};

#endif // !NOJIT

#endif // TRACE_H
