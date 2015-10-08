#ifndef GOTO_H
#define GOTO_H

static void *op_disp_table[] = {
#define op(x) &&x,
#include "opcode.def"
#undef op
    0};

#if !NOJIT
static void *trace_disp_table[] = {
#define op(x) &&trace_##x,
#include "opcode.def"
#undef op
    0};
#endif

#endif
