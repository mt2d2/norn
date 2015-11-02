#include <map>
#include <ostream>
#include <string>
#include <vector>

#include "ir.h"
#include "common.h"

std::ostream &operator<<(std::ostream &stream, const IR::Opcode op) {
  static const std::map<IR::Opcode, std::string> opcodeToString{
      {IR::Opcode::LitInt, "LitInt"},     {IR::Opcode::LoadInt, "LoadInt"},
      {IR::Opcode::StoreInt, "StoreInt"}, {IR::Opcode::LeInt, "LeInt"},
      {IR::Opcode::AddInt, "AddInt"},     {IR::Opcode::MulInt, "MulInt"},
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
