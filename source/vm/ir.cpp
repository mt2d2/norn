#include <map>
#include <ostream>
#include <string>
#include <vector>

#include "ir.h"
#include "common.h"

IR::IR(const Opcode op, const int64_t arg)
    : op(op), ref1(0), ref2(0), intArg(arg), hasConstantArg1(true),
      hasRef1(false), hasRef2(false), variableName(0) {}
IR::IR(const Opcode op, const std::size_t ref1)
    : op(op), ref1(ref1), ref2(0), intArg(0), hasConstantArg1(false),
      hasRef1(true), hasRef2(false), variableName(0) {}
IR::IR(const Opcode op, const std::size_t ref1, const std::size_t ref2)
    : op(op), ref1(ref1), ref2(ref2), intArg(0), hasConstantArg1(false),
      hasRef1(true), hasRef2(true), variableName(0) {}

bool IR::yieldsConstant() const { return op == Opcode::LitInt; }

bool IR::isJump() const { return op == Opcode::Fjmp || op == Opcode::Ujmp; }

bool IR::hasSideEffect() const {
  return op == Opcode::StoreInt || op == Opcode::LoadInt || this->isJump();
}

std::ostream &operator<<(std::ostream &stream, const IR::Opcode op) {
  static const std::map<IR::Opcode, std::string> opcodeToString{
      {IR::Opcode::LitInt, "LitInt"},     {IR::Opcode::LoadInt, "LoadInt"},
      {IR::Opcode::StoreInt, "StoreInt"}, {IR::Opcode::LeInt, "LeInt"},
      {IR::Opcode::AddInt, "AddInt"},     {IR::Opcode::SubInt, "SubInt"},
      {IR::Opcode::MulInt, "MulInt"},     {IR::Opcode::DivInt, "DivInt"},
      {IR::Opcode::Fjmp, "Fjmp"},         {IR::Opcode::Ujmp, "Ujmp"}};

  try {
    return stream << opcodeToString.at(op);
  } catch (const std::out_of_range &err) {
    return stream << "??";
  }
}

std::ostream &operator<<(std::ostream &stream, const IR &ir) {
  std::vector<std::string> args;
  if (ir.hasRef1) {
    args.push_back(std::to_string(ir.ref1));
  }
  if (ir.hasRef2) {
    if (!ir.hasRef2) {
      raise_error("if only one ref, should use first");
    }
    args.push_back(std::to_string(ir.ref2));
  }
  if (ir.hasConstantArg1) {
    args.push_back("k" + std::to_string(ir.intArg));
  }

  stream << ir.variableName << " <- " << ir.op << "\t";
  std::copy(std::begin(args), std::end(args),
            std::ostream_iterator<std::string>(stream, "\t"));
  return stream;
}
