#ifndef TRACE_H
#define TRACE_H

#include <vector>
#include <map>
#include <stack>

#include <asmjit/asmjit.h>

struct Instruction;

class Trace {
public:
  Trace();
  ~Trace();
  void record(const Instruction *i);
  bool is_head(const Instruction *i);
  void debug();
  void jit();

private:
  void load_locals(asmjit::host::Compiler &c, const asmjit::host::GpVar &sp,
                   const asmjit::host::GpVar &mp);
  std::stack<asmjit::host::GpVar> identifyValuesToRetrieveFromLangStack(asmjit::host::Compiler &c);

  std::map<const Instruction *, asmjit::host::GpVar> locals;
  std::vector<const Instruction *> instructions;
};

#endif // TRACE_H
