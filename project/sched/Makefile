CC=gcc
CFLAGS=-g
SRC := $(wildcard *.c)
OBJ := $(patsubst %.c,%.o,$(SRC))
DEP := $(patsubst %.c,%.d,$(SRC))

LIBS= -lpthread 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

libsched.a: $(OBJ)
	ar rcs libsched.a $(OBJ)

clean:
	rm -f libsched.a *.o
