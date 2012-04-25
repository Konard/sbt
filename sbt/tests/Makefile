NAME=test
OSBT=../parent/sbt.o
OPTS=-O3 -std=c99 -I../parent -Wall -g
#OSBT=../woparent/sbt.o
#OPTS=-O3 -std=c99 -I../woparent -Wall


all: 1 2 3 4 5 6 7 8 9 a b c
#all: 1 2 3 4 5 6 7 c

%:	%.c Makefile $(OSBT)
	gcc -c $(OPTS) $<
	gcc -o $* $*.o $(OSBT)

clean:
		rm -f $(BINS)
		rm -f $(OBJS)
