#include <map>
#include <iomanip>
#include <ios>
#include <ostream>
#include <string>
#include <vector>

#include "ir.h"

unsigned int IR::variableNameGen = 0;

IR::IR(const Opcode op)
    : op(op), intArg(0), hasConstantArg1(false),
      variableName(variableNameGen++), deadCode(false),
      references(std::vector<IR *>()) {}
IR::IR(const Opcode op, const int64_t arg)
    : op(op), intArg(arg), hasConstantArg1(true),
      variableName(variableNameGen++), deadCode(false),
      references(std::vector<IR *>()) {}
IR::IR(const Opcode op, IR *ref1)
    : op(op), intArg(0), hasConstantArg1(false),
      variableName(variableNameGen++), deadCode(false),
      references(std::vector<IR *>()) {
  references.push_back(ref1);
}
IR::IR(const Opcode op, IR *ref1, int64_t intArg)
    : op(op), intArg(intArg), hasConstantArg1(true),
      variableName(variableNameGen++), deadCode(false),
      references(std::vector<IR *>()) {
  references.push_back(ref1);
}
IR::IR(const Opcode op, IR *ref1, IR *ref2)
    : op(op), intArg(0), hasConstantArg1(false),
      variableName(variableNameGen++), deadCode(false),
      references(std::vector<IR *>()) {
  references.push_back(ref1);
  references.push_back(ref2);
}

bool IR::hasRef1() const {
  return references.size() >= 1 && references[0] != nullptr;
}
bool IR::hasRef2() const {
  return references.size() >= 2 && references[1] != nullptr;
  ;
}
IR *IR::getRef1() const {
  if (!hasRef1())
    return nullptr;
  return references[0];
}
IR *IR::getRef2() const {
  if (!hasRef2())
    return nullptr;
  return references[1];
}
void IR::setRef1(IR *ref) {
  if (references.size() == 0)
    references.push_back(ref);
  references[0] = ref;
}
void IR::setRef2(IR *ref) {
  if (references.size() == 1)
    references.push_back(ref);
  references[1] = ref;
}

bool IR::yieldsConstant() const { return op == Opcode::LitInt; }

bool IR::isJump() const {
  return op == Opcode::Tjmp || op == Opcode::Fjmp || op == Opcode::Ujmp;
}

bool IR::isLoad() const { return op == Opcode::LoadInt; }

bool IR::isStore() const { return op == Opcode::StoreInt; }

bool IR::hasSideEffect() const {
  return op == Opcode::Loop || isStore() || isLoad() || isJump();
}

bool IR::isPhi() const { return op == Opcode::Phi; }

void IR::clear() {
  op = IR::Opcode::Nop;
  references.clear();
  intArg = 0;
  hasConstantArg1 = 0;
}

std::ostream &operator<<(std::ostream &stream, const IR::Opcode op) {
  static const std::map<IR::Opcode, std::string> opcodeToString{
      {IR::Opcode::LitInt, "LitInt"},
      {IR::Opcode::LoadInt, "LoadInt"},
      {IR::Opcode::StoreInt, "StoreInt"},
      {IR::Opcode::EqInt, "EqInt"},
      {IR::Opcode::NeqInt, "NeqInt"},
      {IR::Opcode::LeInt, "LeInt"},
      {IR::Opcode::LeqInt, "LeqInt"},
      {IR::Opcode::GeInt, "GeInt"},
      {IR::Opcode::AddInt, "AddInt"},
      {IR::Opcode::SubInt, "SubInt"},
      {IR::Opcode::MulInt, "MulInt"},
      {IR::Opcode::ModInt, "ModInt"},
      {IR::Opcode::DivInt, "DivInt"},
      {IR::Opcode::Fjmp, "Fjmp"},
      {IR::Opcode::Tjmp, "Tjmp"},
      {IR::Opcode::Ujmp, "Ujmp"},
      {IR::Opcode::Loop, "Loop"},
      {IR::Opcode::Nop, "Nop"},
      {IR::Opcode::Phi, "Phi"}};

  try {
    return stream << opcodeToString.at(op);
  } catch (const std::out_of_range &err) {
    return stream << "??";
  }
}

std::ostream &operator<<(std::ostream &stream, const IR &ir) {
  std::vector<std::string> args;
  if (ir.hasRef1()) {
    args.push_back(std::to_string(ir.getRef1()->variableName));
  }
  if (ir.hasRef2()) {
    args.push_back(std::to_string(ir.getRef2()->variableName));
  }
  if (ir.hasConstantArg1) {
    args.push_back("k" + std::to_string(ir.intArg));
  }

  stream << std::left << std::setw(3) << ir.variableName << "<- " << ir.op
         << "\t";
  std::copy(std::begin(args), std::end(args),
            std::ostream_iterator<std::string>(stream, "\t"));
  return stream;
}
