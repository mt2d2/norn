CC=clang++
ECHO=/bin/echo

EXE=norn
SRC=source/vm/common.cpp source/vm/opcode.cpp source/vm/memory.cpp source/vm/block.cpp source/vm/optimizer.cpp source/vm/jit.cpp source/vm/program.cpp  \
 	source/vm/machine.cpp source/tree.cpp source/generate.cpp source/type.cpp source/lexer.cpp source/parser.cpp source/main.cpp 
OBJ=${SRC:.cpp=.o}

LIBASMJIT=source/vm/AsmJit/libasmjit.a

# PGO: use -fprofile-generate, run then -fprofile-use; in both
# Other: -fomit-frame-pointer -pipe -march=core2 -g -fast -stdlib=libc++
CFLAGS=-std=c++11 -Wall -Werror -pipe -g -O2
LDFLAGS=

all: ${SRC} ${EXE}

${EXE}: ${OBJ} ${LIBASMJIT}
	@${ECHO} LINK $@
	@${CC} ${LDFLAGS} ${OBJ} ${LIBASMJIT} -o $@ ${LIB}

${LIBASMJIT}:
	make -C source/vm/AsmJit

.cpp.o:
	@${ECHO} CC $<
	@${CC} ${CFLAGS} -c $< -o $@

clean:
	@${ECHO} RM objects, executable
	@rm -f ${OBJ} ${EXE}

realclean: clean
	@${ECHO} RM profile data, libasmjit.a
	@rm -f source/*gc* source/vm/*gc*
	@make -C source/vm/AsmJit clean


test:
	python test/runner.py

.PHONY: clean realclean test

