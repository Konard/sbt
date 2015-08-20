@REM without -O0 optimization, see stack arguments

D:\PROGS\TDM-GCC-64\bin\gcc.exe -march=native -m64 -std=c99 -O0 -c -o test.obj test.c
D:\PROGS\TDM-GCC-64\bin\gcc.exe -march=native -m64 -std=c99 -O0 -c -o sbt.obj sbt.c
D:\UTILS\yasm.exe -f coff -m amd64 -o func.obj func.asm
D:\PROGS\TDM-GCC-64\bin\gcc.exe -o test.exe test.obj sbt.obj func.obj

D:\PROGS\TDM-GCC-64\bin\gcc.exe -S -march=native -m64 -std=c99 -O0 -c -o sbt.S sbt.c
D:\PROGS\TDM-GCC-64\bin\gcc.exe -S -march=native -m64 -std=c99 -O0 -c -o test.S test.c

@REM D:\UTILS\yasm.exe -w -f bin -m amd64 -o func.bin func.asm
@REM D:\UTILS\ndisasm.exe func.bin > func.S

@REM D:\PROGS\TDM-GCC-64\bin\strip.exe --strip-all test.exe
@REM D:\UTILS\upx.exe -q -9 test.exe
