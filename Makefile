CFLAGS=-O3
LIBS=-lzmq
CC=gcc

zmq-monitor: zmq-monitor.c
	$(CC) -o $@ $< $(CFLAGS) $(LIBS)

clean:
	rm -f zmq-monitor

dev: CFLAGS=-g -Wall -Wextra
dev: zmq-monitor

new: clean zmq-monitor

.PHONY: clean
