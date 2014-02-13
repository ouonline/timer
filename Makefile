CC := gcc
CFLAGS := -O2 -Wall -Werror

LIBS := -lpthread

OBJS := $(patsubst %c, %o, $(wildcard *.c) ../threadpool/threadpool.c)
TARGET := alarm-timer-test pthread-timer-test

.PHONY: all clean

all: $(OBJS) $(TARGET)

alarm-timer-test: test-timer.o alarm-timer.o ../threadpool/threadpool.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

pthread-timer-test: test-timer.o pthread-timer.o ../threadpool/threadpool.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)
