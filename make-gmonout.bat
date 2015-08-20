D:\PROGS\TDM-GCC-64\bin\gcc.exe -mfentry -g3 -pg -static -march=native -m64 -std=c99 -O0 -c -o test.obj test.c
D:\PROGS\TDM-GCC-64\bin\gcc.exe -mfentry -g3 -pg -static -march=native -m64 -std=c99 -O0 -c -o sbt.obj sbt.c
D:\UTILS\yasm.exe -f coff -m amd64 -o func.obj func.asm
D:\PROGS\TDM-GCC-64\bin\gcc.exe -mfentry -g3 -pg -static -o test.exe test.obj sbt.obj func.obj
