server: server.o
	cc server.o -LS -o server
server.o: server.c server.h
	cc -c server.c

clean:
	rm *.o
