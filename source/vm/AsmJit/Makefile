CC=clang++
ECHO=/bin/echo
AR=ar rcs

LIB=libasmjit.a

SRC=AssemblerX86X64.cpp CodeGenerator.cpp Compiler.cpp \
	CompilerX86X64.cpp CpuInfo.cpp Defs.cpp DefsX86X64.cpp \
	Logger.cpp MemoryManager.cpp OperandX86X64.cpp\
	Platform.cpp Util.cpp
OBJ=${SRC:.cpp=.o}

CFLAGS=-pipe -g -O2
LDFLAGS=

all: ${SRC} ${LIB}

${LIB}: ${OBJ}
	@${ECHO} AR $@
	@ar rcs ${LIB} ${OBJ}

.cpp.o:
	@${ECHO} CC $<
	@${CXX} ${CFLAGS} -c $< -o $@

clean:
	@${ECHO} RM objects, executable
	@rm -f ${OBJ} ${LIB}

.PHONY: clean realclean

