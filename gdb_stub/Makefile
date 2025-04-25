CFLAGS = -std=c99 -pedantic -Wall -I../src

all: stub

stub: main.c rsp.h ../src/rv32.h
	$(CC) $(CFLAGS) $< -o $@

.PHONY: run clean

run: stub
	socat -d -d -v pty,rawer,link=./serial EXEC:"./stub",pty,rawer	

clean:
	rm -f stub
