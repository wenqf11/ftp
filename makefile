prog: server.o
gcc server.o -LS -o prog 
server.o: server.c server.h defs
gcc -c server.c

clean:
	rm *.o
