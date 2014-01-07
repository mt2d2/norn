CC=clang
ECHO=/bin/echo
AR=ar rcs

LIB=dlmalloc.a

SRC=dlmalloc.c
OBJ=${SRC:.c=.o}

CFLAGS=-pipe -g -O3 -DUSE_DL_PREFIX
LDFLAGS=

all: ${SRC} ${LIB}

${LIB}: ${OBJ}
	@${ECHO} AR $@
	@ar rcs ${LIB} ${OBJ}

.c.o:
	@${ECHO} CC $<
	@${CC} ${CFLAGS} -c $< -o $@

clean:
	@${ECHO} RM objects, executable
	@rm -f ${OBJ} ${LIB}

.PHONY: clean realclean

