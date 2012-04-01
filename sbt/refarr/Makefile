OPTS=-O3 -std=c99 -g -Wall -DSBT_ARRAYS

sbt.o:		sbt.c sbt.h Makefile
		gcc ${OPTS} -c -o sbt.o sbt.c

clean:
		rm -f sbt.o
