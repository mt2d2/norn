#ifndef GOTO_H
#define GOTO_H

static void *disp_table[] = {
#define op(x) &&x,
#include "opcode.def"
#undef op
  0
};

#endif
