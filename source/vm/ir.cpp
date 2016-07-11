#include <map>
#include <cassert>
#include <iomanip>
#include <ios>
#include <iterator>
#include <ostream>
#include <string>
#include <vector>

#include "ir.h"

unsigned IR::variableNameGen = 0;

IR::IR(const Opcode op) : IR::IR(op, nullptr, nullptr, nullptr, false, 0) {}
IR::IR(const Opcode op, const int64_t arg)
    : IR::IR(op, nullptr, nullptr, nullptr, true, arg) {}
IR::IR(const Opcode op, IR *ref1)
    : IR::IR(op, ref1, nullptr, nullptr, false, 0) {}
IR::IR(const Opcode op, IR *ref1, int64_t intArg)
    : IR::IR(op, ref1, nullptr, nullptr, true, intArg) {}
IR::IR(const Opcode op, IR *ref1, IR *ref2)
    : IR::IR(op, ref1, ref2, nullptr, false, 0) {}
IR::IR(const Opcode op, IR *ref1, IR *ref2, IR *ref3)
    : IR::IR(op, ref1, ref2, ref3, false, 0) {}

IR::IR(const Opcode op, IR *ref1, IR *ref2, IR *ref3, bool constant,
       int64_t intArg)
    : op(op), intArg(intArg), hasConstantArg1(constant),
      variableName(variableNameGen++), deadCode(false),
      references(std::vector<IR *>()) {
  if (ref1 != nullptr)
    references.push_back(ref1);
  if (ref2 != nullptr)
    references.push_back(ref2);
  if (ref3 != nullptr)
    references.push_back(ref3);
}

bool IR::hasRef1() const {
  return references.size() >= 1 && references[0] != nullptr;
}
bool IR::hasRef2() const {
  return references.size() >= 2 && references[1] != nullptr;
}
bool IR::hasRef3() const {
  return references.size() >= 3 && references[2] != nullptr;
}
IR *IR::getRef1() const {
  assert(hasRef1());
  return references[0];
}
IR *IR::getRef2() const {
  assert(hasRef2());
  return references[1];
}
IR *IR::getRef3() const {
  assert(hasRef3());
  return references[2];
}
void IR::setRef1(IR *ref) {
  assert(ref != nullptr);
  if (references.size() == 0)
    references.push_back(ref);
  else
    references[0] = ref;
}
void IR::setRef2(IR *ref) {
  assert(ref != nullptr);
  if (references.size() == 1)
    references.push_back(ref);
  else
    references[1] = ref;
}

void IR::pushBackRef(IR *ref) { references.push_back(ref); }

void IR::removeRef1() {
  assert(hasRef1());
  references.erase(references.begin());
}
void IR::removeRef2() {
  assert(hasRef2());
  references.erase(references.begin() + 1);
}
void IR::removeRef3() {
  assert(hasRef3());
  references.erase(references.begin() + 2);
}

bool IR::yieldsConstant() const { return op == Opcode::LitInt; }

bool IR::isJump() const {
  return op == Opcode::Tjmp || op == Opcode::Fjmp || op == Opcode::Ujmp;
}

bool IR::isLoad() const { return op == Opcode::LoadInt; }

bool IR::isRefLoad() const { return op == Opcode::RefLoadInt; }

bool IR::isStore() const { return op == Opcode::StoreInt; }

bool IR::isRefStore() const { return op == Opcode::RefStoreInt; }

bool IR::hasSideEffect() const {
  return op == Opcode::Loop || isLoad() || isRefLoad() || isStore() ||
         isRefStore() || isJump();
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
      {IR::Opcode::Aref, "Aref"},
      {IR::Opcode::RefLoadInt, "RefLoadInt"},
      {IR::Opcode::RefStoreInt, "RefStoreInt"},
      {IR::Opcode::Fjmp, "Fjmp"},
      {IR::Opcode::Tjmp, "Tjmp"},
      {IR::Opcode::Ujmp, "Ujmp"},
      {IR::Opcode::Loop, "Loop"},
      {IR::Opcode::Nop, "Nop"},
      {IR::Opcode::Phi, "Phi"}};

  try {
    return stream << opcodeToString.at(op);
  } catch (const std::out_of_range &err) {
    return stream << "?? " << err.what();
  }
}

std::ostream &operator<<(std::ostream &stream, const IR &ir) {
  std::vector<std::string> args;
  for (const auto *ref : ir.references) {
    assert(ref != nullptr);
    args.push_back(std::to_string(ref->variableName));
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
