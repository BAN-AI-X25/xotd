# Make for x25 server

xotd:	xotd2.o config.o log.o outbound.o inbound.o utility.o findfor.o \
	x25trace.o loging.o leave_pid.o sighandler.o all.h
	gcc -g -O -Wall -D_REENTRANT -lpthread -o xotd xotd2.o config.o log.o outbound.o inbound.o utility.o findfor.o \
	x25trace.o loging.o leave_pid.o sighandler.o

clear:
	rm xotd2.o config.o log.o outbound.o inbound.o utility.o findfor.o \
	x25trace.o loging.o leave_pid.o sighandler.o
