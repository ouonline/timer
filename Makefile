CC := gcc

ifeq ($(release), y)
    CFLAGS := -O2 -DNDEBUG
else
    CFLAGS := -g
endif

CFLAGS := $(CFLAGS) -Wall -Werror

LIBS := -lpthread

OBJS := $(patsubst %c, %o, $(wildcard *.c) ../mythreadpool/c/threadpool.c)

TARGET := test_alarm_timer test_pthread_timer

.PHONY: all clean

all: $(OBJS) $(TARGET)

test_alarm_timer: test_timer.o alarm_timer.o ../mythreadpool/c/threadpool.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

test_pthread_timer: test_timer.o pthread_timer.o ../mythreadpool/c/threadpool.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)
