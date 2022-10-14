CFLAGS = -std=c99 -pedantic -Wall

all: stub

stub: main.c
	$(CC) $(CFLAGS) $< -o $@

.PHONY: run clean

run: stub
	socat -d -d -v pty,rawer,link=./serial EXEC:"./stub",pty,rawer	

clean:
	rm -f stub
