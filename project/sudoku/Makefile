CC=gcc
CFLAGS=-g

SCHED= ../sched
INCLUDE= -I. -I $(SCHED)
CFLAGS += $(INCLUDE)

SRC := $(wildcard *.c)
OBJ := $(patsubst %.c,%.o,$(SRC))
DEP := $(patsubst %.c,%.d,$(SRC))

LIBS= -L. \
      -L $(SCHED) \
      -lsched \
      -lpthread

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

sudoku: $(OBJ)
	$(MAKE) -C $(SCHED)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -f sudoku *.o
	$(MAKE) -C $(SCHED) clean
