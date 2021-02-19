CC=gcc
CFLAGS=-g

SCHED= ../sched
INCLUDE= -I. -I $(SCHED)
CFLAGS += $(INCLUDE)

ifeq ($(PTHREAD_SYNC), 1)
	CFLAGS += -DPTHREAD_SYNC
endif

SRC := $(wildcard *.c)
OBJ := $(patsubst %.c,%.o,$(SRC))
DEP := $(patsubst %.c,%.d,$(SRC))

LIBS= -L. \
      -L $(SCHED) \
      -lsched \
      -lpthread

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

simple_thread: $(OBJ)
	$(MAKE) -C $(SCHED)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -f simple_thread *.o
	$(MAKE) -C $(SCHED) clean
