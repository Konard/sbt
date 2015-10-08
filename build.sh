/usr/bin/gcc -m64 -Ofast -flto -march=native -funroll-loops -std=gnu89 -c -o test.o test.c
/usr/bin/gcc -m64 -Ofast -flto -march=native -funroll-loops -std=gnu89 -c -o sbt.o sbt.c
/usr/bin/gcc -o test test.o sbt.o

/usr/bin/gcc -Ofast -std=gnu89 -c -S -o sbt.S sbt.c
/usr/bin/gcc -Ofast -std=gnu89 -c -S -o test.o test.c
