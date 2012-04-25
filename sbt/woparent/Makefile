#OPTS=-O3 -std=c99 -Wall -DSBT_ARRAYS
OPTS=-O3 -std=c99 -Wall

sbt.o:		sbt.c sbt.h Makefile
		gcc ${OPTS} -c -o sbt.o sbt.c

clean:
		rm -f sbt.o
