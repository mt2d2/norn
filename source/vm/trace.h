#ifndef TRACE_H
#define TRACE_H

#include <vector>

struct Instruction;

class Trace {
public:
  Trace();
  ~Trace();
  void record(const Instruction *i);
  bool is_head(const Instruction* i);
  void debug();

private:
  std::vector<const Instruction *> instructions;
};

#endif // TRACE_H
