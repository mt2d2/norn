CXX=clang++
ECHO=/bin/echo
CMAKE=cmake

EXE=norn
SRC=source/vm/common.cpp source/vm/opcode.cpp source/vm/memory.cpp source/vm/block.cpp source/vm/optimizer.cpp source/vm/program.cpp  \
 	source/vm/trace.cpp source/vm/machine.cpp source/tree.cpp source/generate.cpp source/type.cpp source/lexer.cpp source/parser.cpp source/main.cpp
OBJ=${SRC:.cpp=.o}

LIBASMJIT=source/vm/AsmJit/libasmjit.a
LIBDLMALLOC=source/vm/dlmalloc/dlmalloc.a

CFLAGS=-std=c++11 -Wall -g -O2
${EXE}_nojit: CFLAGS += -DNOJIT=1

CFLAGS += -Isource/vm/dlmalloc -Isource/vm/asmjit/src -DUSE_DL_PREFIX

all: ${SRC} ${EXE}

${EXE}: ${OBJ} ${LIBASMJIT} ${LIBDLMALLOC}
	@${ECHO} LINK $@
	@${CXX} ${LDFLAGS} ${OBJ} ${LIBASMJIT} ${LIBDLMALLOC} -o $@ ${LIB}

${EXE}_nojit: ${OBJ} ${LIBDLMALLOC}
	@${ECHO} LINK $@
	@${CXX} ${LDFLAGS} ${OBJ} ${LIBDLMALLOC} -o ${EXE} ${LIB}

${LIBASMJIT}:
	@${ECHO} MAKE libasmjit
	@cd source/vm/asmjit && ${CMAKE} -DASMJIT_STATIC=1
	@+${MAKE} -C source/vm/asmjit

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
	@${ECHO} RM profile data, libasmjit.a, libdlmalloc.a
	@rm -f source/*gc* source/vm/*gc*
	@make -C source/vm/asmjit clean
	@rm -f source/vm/asmjit/CMakeCache.txt
	@rm -rf source/vm/asmjit/CMakeFiles
	@rm -rf source/vm/asmjit/cmake_install.cmake
	@rm -rf source/vm/asmjit/Makefile
	@make -C source/vm/dlmalloc clean

test: ${EXE}
	python test/runner.py

format:
	find . -name '*.cpp' ! -path "./source/vm/asmjit/*" ! -path "./source/vm/dlmalloc/*" -print0 | xargs -0 -n 1 clang-format -i -style llvm
	find . -name '*.h' ! -path "./source/vm/asmjit/*" ! -path "./source/vm/dlmalloc/*" -print0 | xargs -0 -n 1 clang-format -i -style llvm

.PHONY: clean realclean test format
