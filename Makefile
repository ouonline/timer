CC := gcc

ifeq ($(debug), y)
    CFLAGS := -g
else
    CFLAGS := -O2 -DNDEBUG
endif

CFLAGS := $(CFLAGS) -Wall -Werror

ifndef DEPSDIR
    DEPSDIR := $(shell pwd)/..
endif

INCLUDE := -I. -I$(DEPSDIR)
LIBS := $(DEPSDIR)/threadpool/c/libthreadpool.a -lpthread

OBJS := $(patsubst %c, %o, $(wildcard *.c))

TARGET := test_alarm_timer test_pthread_timer

.PHONY: all clean pre-process post-clean

all: $(TARGET)

$(OBJS): | pre-process

pre-process:
	d=$(DEPSDIR)/utils; if ! [ -d $$d ]; then git clone https://github.com/ouonline/utils.git $$d; fi
	d=$(DEPSDIR)/threadpool; if ! [ -d $$d ]; then git clone https://github.com/ouonline/threadpool.git $$d; fi
	$(MAKE) DEPSDIR=$(DEPSDIR) -C $(DEPSDIR)/threadpool/c

post-clean:
	$(MAKE) clean DEPSDIR=$(DEPSDIR) -C $(DEPSDIR)/threadpool/c

test_alarm_timer: test_timer.o alarm_timer.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

test_pthread_timer: test_timer.o pthread_timer.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

clean: | post-clean
	rm -f $(TARGET) $(OBJS)
