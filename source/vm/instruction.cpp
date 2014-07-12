#include "instruction.h"

bool is_condition(const long op) {
  return op == LE_INT || op == LEQ_INT || op == GE_INT || op == GEQ_INT ||
         op == EQ_INT || op == NEQ_INT || op == LE_FLOAT || op == LEQ_FLOAT ||
         op == GE_FLOAT || op == GEQ_FLOAT || op == EQ_FLOAT || op == NEQ_FLOAT;
}

bool is_guard(const long op) { return op == TJMP || op == FJMP; }
