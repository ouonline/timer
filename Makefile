CC := gcc
CFLAGS := -Wall -g
LIBS := -lpthread

TARGET := alarm-timer-test pthread-timer-test

.PHONY: all clean

all: $(TARGET)

alarm-timer-test: test-timer.o alarm-timer.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

pthread-timer-test: test-timer.o pthread-timer.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGET) *.o
