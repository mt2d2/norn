CXX=clang++
ECHO=/bin/echo

EXE=norn
SRC=source/vm/common.cpp source/vm/opcode.cpp source/vm/memory.cpp source/vm/block.cpp source/vm/optimizer.cpp source/vm/jit.cpp source/vm/program.cpp  \
 	source/vm/machine.cpp source/tree.cpp source/generate.cpp source/type.cpp source/lexer.cpp source/parser.cpp source/main.cpp 
OBJ=${SRC:.cpp=.o}

LIBASMJIT=source/vm/AsmJit/libasmjit.a
LIBDLMALLOC=source/vm/dlmalloc/dlmalloc.a

CFLAGS=-std=c++11 -Wall -Werror -pipe -g -O2
${EXE}_nojit: CFLAGS += -DNOJIT=1

CFLAGS += -Isource/vm/dlmalloc -DUSE_DL_PREFIX

all: ${SRC} ${EXE}

${EXE}: ${OBJ} ${LIBASMJIT} ${LIBDLMALLOC}
	@${ECHO} LINK $@
	@${CXX} ${LDFLAGS} ${OBJ} ${LIBASMJIT} ${LIBDLMALLOC} -o $@ ${LIB}

${EXE}_nojit: ${OBJ}
	@${ECHO} LINK $@
	@${CXX} ${LDFLAGS} ${OBJ} -o ${EXE} ${LIB}

${LIBASMJIT}:
	@${ECHO} MAKE libasmjit
	@+${MAKE} -C source/vm/AsmJit

${LIBDLMALLOC}:
	@${ECHO} MAKE libdlmalloc
	@+${MAKE} -C source/vm/dlmalloc

.cpp.o:
	@${ECHO} CXX $<
	@${CXX} ${CFLAGS} -c $< -o $@

clean:
	@${ECHO} RM objects, executable
	@rm -f ${OBJ} ${EXE}

realclean: clean
	@${ECHO} RM profile data, libasmjit.a
	@rm -f source/*gc* source/vm/*gc*
	@make -C source/vm/AsmJit clean

test: ${EXE}
	python test/runner.py

.PHONY: clean realclean test

