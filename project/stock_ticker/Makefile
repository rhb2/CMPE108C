CC=gcc
CFLAGS=-g

SCHED= ../sched
INCLUDE= -I. -I $(SCHED)
CFLAGS += $(INCLUDE)

SRC := $(wildcard *.c ../sched/*.c)
OBJ := $(patsubst %.c,%.o,$(SRC))
DEP := $(patsubst %.c,%.d,$(SRC))

LIBS= -L. \
      -L ../sched \
      -lpthread \
      -lnsl \
      -lssl \
      -lcrypto \
      -lcurl \
      -lsched

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

stockwatch: $(OBJ)
	$(MAKE) -C $(SCHED)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -f stockwatch *.o
	$(MAKE) -C $(SCHED) clean
