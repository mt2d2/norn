CC=clang++
ECHO=/bin/echo

EXE=norn
SRC=source/vm/common.cpp source/vm/opcode.cpp source/vm/memory.cpp source/vm/block.cpp source/vm/optimizer.cpp source/vm/jit.cpp source/vm/program.cpp  \
 	source/vm/machine.cpp source/tree.cpp source/generate.cpp source/type.cpp source/lexer.cpp source/parser.cpp source/main.cpp \
	\
	source/vm/AsmJit/Assembler.cpp source/vm/AsmJit/AssemblerX86X64.cpp source/vm/AsmJit/CodeGenerator.cpp source/vm/AsmJit/Compiler.cpp \
	source/vm/AsmJit/CompilerX86X64.cpp source/vm/AsmJit/CpuInfo.cpp source/vm/AsmJit/Defs.cpp source/vm/AsmJit/DefsX86X64.cpp \
	source/vm/AsmJit/Logger.cpp source/vm/AsmJit/MemoryManager.cpp source/vm/AsmJit/Operand.cpp source/vm/AsmJit/OperandX86X64.cpp\
	source/vm/AsmJit/Platform.cpp source/vm/AsmJit/Util.cpp
OBJ=${SRC:.cpp=.o}
LIB=
INC=

# PGO: use -fprofile-generate, run then -fprofile-use; in both
# Other: -fomit-frame-pointer -pipe -march=core2 -g -fast -stdlib=libc++
CFLAGS=-Wall -pipe -g -O3
LDFLAGS=-rdynamic

all: ${SRC} ${EXE}

${EXE}: ${OBJ}
	@${ECHO} LINK $@
	@${CC} ${LDFLAGS} ${OBJ} -o $@ ${LIB}

.cpp.o:
	@${ECHO} CC $<
	@${CC} -I${INC} ${CFLAGS}  -c $< -o $@

clean:
	@${ECHO} RM objects, executable
	@rm -f ${OBJ} ${EXE}

realclean: clean
	@${ECHO} RM profile data
	@rm -f source/*gc* source/vm/*gc*

test:
	python test/runner.py

.PHONY: clean realclean test

