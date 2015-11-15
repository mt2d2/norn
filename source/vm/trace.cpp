#include "trace.h"

#if !NOJIT

#include <algorithm>
#include <iostream>
#include <map>
#include <stack>
#include <string>
#include <unordered_set>

#include "common.h"
#include "block.h"
#include "program.h"
#include "instruction.h"

Trace::Trace()
    : last_state(Trace::State::ABORT),
      bytecode(std::vector<const Instruction *>()),
      instructions(std::list<IR>()), phisFor(std::map<int64_t, LoadForPhi>()),
      traceExits(std::vector<uint64_t>()), nativePtr(nullptr) {}

Trace::~Trace() {
  if (nativePtr != nullptr)
    raise_error("nativePtr should remain nullptr");
}

Trace::State Trace::record(const Instruction *i) {
  if (isHead(i)) {
    last_state = Trace::State::COMPLETE;
    goto exit;
  }

  if (i->op == LOOP && bytecode.size() > 0 && i->arg.l != bytecode[0]->arg.l) {
    // encountered another loop, abort
    last_state = Trace::State::ABORT;
    goto exit;
  }

  bytecode.push_back(i);
  last_state = Trace::State::TRACING;

exit:
  return last_state;
}

bool Trace::is_complete() const { return last_state == Trace::State::COMPLETE; }

bool Trace::isHead(const Instruction *i) const {
  return bytecode.size() > 0 && i == bytecode[0];
}

void Trace::debug() const {
  std::cout << "Bytecode: " << std::endl;
  for (const auto *i : bytecode) {
    std::cout << *i << std::endl;
  }

  std::cout << "IR: " << std::endl;
  std::size_t i = 1;
  for (const auto &ir : instructions) {
    if (!ir.deadCode)
      std::cout << ir << std::endl;
  }
}

struct Frame {
  std::stack<IR *> stack;
  std::map<int64_t, IR *> memory;
};

void Trace::convertBytecodeToIR() {
  std::stack<Frame> frames;
  frames.push(Frame{});
  Frame &frame = frames.top();

  for (const auto *instr : bytecode) {
    switch (instr->op) {
    case LIT_INT: {
      instructions.emplace_back(IR(IR::Opcode::LitInt, instr->arg.l));
      frame.stack.push(&instructions.back());
    } break;

    case LOAD_INT: {
      instructions.emplace_back(IR(IR::Opcode::LoadInt, instr->arg.l));
      frame.stack.push(&instructions.back());
      frame.memory[instr->arg.l] = &instructions.back();
    } break;

    case STORE_INT: {
      auto *ir = frame.stack.top();
      frame.stack.pop();
      frame.memory[instr->arg.l] = ir;
      instructions.emplace_back(IR(IR::Opcode::StoreInt, ir, instr->arg.l));
    } break;

    case ADD_INT:
    case SUB_INT:
    case MUL_INT:
    case DIV_INT:
    case MOD_INT:
    case EQ_INT:
    case NEQ_INT:
    case LE_INT:
    case LEQ_INT:
    case GE_INT: {
      auto *ir1 = frame.stack.top();
      frame.stack.pop();
      auto *ir2 = frame.stack.top();
      frame.stack.pop();

      static const std::map<Opcode, IR::Opcode> bytecodeToIR{
          {ADD_INT, IR::Opcode::AddInt}, {SUB_INT, IR::Opcode::SubInt},
          {MUL_INT, IR::Opcode::MulInt}, {DIV_INT, IR::Opcode::SubInt},
          {MOD_INT, IR::Opcode::ModInt}, {EQ_INT, IR::Opcode::EqInt},
          {NEQ_INT, IR::Opcode::NeqInt}, {LE_INT, IR::Opcode::LeInt},
          {LEQ_INT, IR::Opcode::LeqInt}, {GE_INT, IR::Opcode::GeInt}};
      const auto irOp = bytecodeToIR.find(static_cast<Opcode>(instr->op));
      if (irOp == bytecodeToIR.end())
        raise_error("unknown conversion from bytecode binop to ir binop");

      instructions.emplace_back(IR(irOp->second, ir1, ir2));
      frame.stack.push(&instructions.back());
    } break;

    case FJMP: {
      auto *ir1 = frame.stack.top();
      frame.stack.pop();

      instructions.emplace_back(
          IR(IR::Opcode::Fjmp, ir1)); /* TODO, add jump location */
    } break;
    case TJMP: {
      auto *ir1 = frame.stack.top();
      frame.stack.pop();

      instructions.emplace_back(
          IR(IR::Opcode::Tjmp, ir1)); /* TODO, add jump location */
    } break;
    case UJMP: { /* pass */
    } break;

    case CALL: {
      frames.push(Frame{});
      frame = frames.top();
    } break;
    case RTRN: {
      frames.pop();
      frame = frames.top();
    } break;

    case LOOP: { /* pass */
    } break;

    default: {
      std::cout << *instr << std::endl;
      raise_error("unknown op in trace compilation");
    } break;
    }
  }
}

enum class WhichRef { Ref1, Ref2 };

void Trace::propagateConstants() {
  const auto fold = [](IR &instr, const int64_t val) {
    switch (instr.op) {
    case IR::Opcode::AddInt:
      instr.intArg += val;
      break;
    case IR::Opcode::SubInt:
      instr.intArg -= val;
      break;
    case IR::Opcode::MulInt:
      instr.intArg *= val;
      break;
    case IR::Opcode::DivInt:
      instr.intArg /= val;
      break;
    default:
      std::cout << instr << std::endl;
      raise_error("unknown integer fold!");
      break;
    }
    instr.op = IR::Opcode::LitInt;
  };

  const auto useConstant = [&fold](IR &instr, const WhichRef whichRef,
                                   const int64_t val) {
    if (!instr.isStore()) {
      if (whichRef == WhichRef::Ref1) {
        instr.removeRef1();
      } else {
        instr.removeRef2();
      }

      if (!instr.hasConstantArg1) {
        instr.hasConstantArg1 = true;
        instr.intArg = val;
      } else {
        fold(instr, val);
      }
    }
  };

  std::vector<IR *> worklist;
  for (auto &instr : instructions)
    worklist.push_back(&instr);

  while (!worklist.empty()) {
    const auto *stmt = worklist.back();
    worklist.pop_back();

    if (stmt->yieldsConstant()) {
      for (auto &instr : instructions) {
        if (instr.hasRef1() &&
            instr.getRef1()->variableName == stmt->variableName) {
          useConstant(instr, WhichRef::Ref1, stmt->intArg);
          worklist.push_back(&instr);
        }
        if (instr.hasRef2() &&
            instr.getRef2()->variableName == stmt->variableName) {
          useConstant(instr, WhichRef::Ref2, stmt->intArg);
          worklist.push_back(&instr);
        }
      }
    }
  }
}

void Trace::eliminateDeadCode() {
  std::unordered_set<const IR *> seenRefs;
  seenRefs.insert(&instructions.back());

  const auto phiMergesRef = [](const IR *phi, const IR *ref) {
    if (!phi->isPhi())
      raise_error("must be phi!");
    return (phi->hasRef1() && phi->getRef1() == ref) ||
           (phi->hasRef2() && phi->getRef2() == ref);
  };

  const auto anyPhiHasRef = [this, &phiMergesRef](const IR *ref) {
    return std::any_of(std::begin(phisFor), std::end(phisFor),
                       [=](const std::pair<int64_t, LoadForPhi> &entry) {
                         return phiMergesRef(entry.second.phi, ref);
                       });
  };

  for (auto it = instructions.rbegin(); it != instructions.rend(); ++it) {
    auto &instr = *it;
    if (seenRefs.count(&instr) != 1 && !instr.hasSideEffect() &&
        !anyPhiHasRef(&instr)) {
      instr.deadCode = true;
    } else if (instr.op == IR::Opcode::Nop) {
      instr.deadCode = true;
    } else {
      if (instr.hasRef1()) {
        seenRefs.insert(instr.getRef1());
      }
      if (instr.hasRef2()) {
        seenRefs.insert(instr.getRef2());
      }
    }
  }
}

void Trace::hoistLoads() {
  const auto reassignRef = [](IR &instr, WhichRef whichRef, IR *n) {
    if (!instr.isPhi()) {
      if (whichRef == WhichRef::Ref1)
        instr.setRef1(n);
      else
        instr.setRef2(n);
    }
  };

  const auto replaceRefs = [this, &reassignRef](IR *o, IR *n) {
    std::for_each(std::begin(instructions), std::end(instructions),
                  [this, &reassignRef, &o, &n](IR &instr) {
                    if (instr.hasRef1() && instr.getRef1() == o)
                      reassignRef(instr, WhichRef::Ref1, n);
                    if (instr.hasRef2() && instr.getRef2() == o)
                      reassignRef(instr, WhichRef::Ref2, n);
                  });
  };

  for (auto &instr : instructions) {
    if (instr.isLoad()) {
      const auto it = phisFor.find(instr.intArg);
      if (it == phisFor.end()) {
        instructions.emplace_front(IR(IR::Opcode::Phi, &instr));
        phisFor[instr.intArg] = LoadForPhi{&instr, &instructions.front()};
        replaceRefs(&instr, &instructions.front());
      } else {
        auto *phi = it->second.phi;
        if (phi->hasRef2())
          raise_error("need more than two slots for phi");
        replaceRefs(&instr, phi);
        instr.clear();
      }
    }
  }

  instructions.emplace_front(IR(IR::Opcode::Loop));
  auto &loopInstr = instructions.front();
  for (auto pair : phisFor) {
    auto *load = pair.second.load;
    instructions.push_front(*load);
    replaceRefs(load, &instructions.front());
    load->clear();
  }
  instructions.emplace_back(IR(IR::Opcode::Ujmp, &loopInstr));
}

void Trace::sinkStores() {
  std::vector<IR> instructionsToAdd;
  for (auto it = std::begin(instructions); it != std::end(instructions); ++it) {
    auto &instr = *it;
    if (instr.isStore()) {
      auto &prevInstr = *std::prev(it, 1);

      const auto phiRecord = phisFor.find(instr.intArg);
      if (phiRecord == phisFor.end())
        raise_error("couldn't find phi record for store: k" +
                    std::to_string(instr.intArg));

      auto *phi = phiRecord->second.phi;
      if (phi->hasRef2())
        raise_error("phis can only have two refs");
      phi->setRef2(&prevInstr);

      instructionsToAdd.emplace_back(
          IR(IR::Opcode::StoreInt, phi, instr.intArg));
      instr.clear();
    }
  }

  instructions.insert(std::end(instructions), std::begin(instructionsToAdd),
                      std::end(instructionsToAdd));
}

void Trace::compile(const bool debug) {
  convertBytecodeToIR();
  propagateConstants();
  hoistLoads();
  sinkStores();
  eliminateDeadCode();
}

nativeTraceType Trace::get_native_ptr() const { return nativePtr; }

uint64_t Trace::get_trace_exit(int offset) const { return traceExits[offset]; }

void Trace::identify_trace_exits() {
  unsigned int bytecodePosition = 0;

  for (const auto *i : bytecode) {
    if (i->op == FJMP || i->op == TJMP) {
      traceExits.push_back(bytecodePosition);
    }

    ++bytecodePosition;
  }
}

#endif // !NOJIT
