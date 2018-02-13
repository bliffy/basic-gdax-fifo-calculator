
CC:=g++

all: enums.o rawin.o
	$(CC) enums.o rawin.o -o runfifo
clean:
	rm -fv enums.o rawin.o runfifo

.c.o:
	$(CC) -c $< -o $@
.cc.o:
	$(CC) -c $< -o $@
