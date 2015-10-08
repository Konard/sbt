@REM without -O3 optimization, see stack arguments

@C:\TDM-GCC-64\bin\gcc.exe -march=native -m64 -std=c99 -Ofast -c -o test.obj test.c
@C:\TDM-GCC-64\bin\gcc.exe -march=native -m64 -std=c99 -Ofast -c -o sbt.obj sbt.c
@D:\UTILS\yasm.exe -f coff -m amd64 -o func.obj func.asm
@C:\TDM-GCC-64\bin\gcc.exe -o test.exe test.obj sbt.obj func.obj

@C:\TDM-GCC-64\bin\gcc.exe -S -march=native -m64 -std=c99 -O3 -c -o test.S test.c

@REM D:\UTILS\yasm.exe -w -f bin -m amd64 -o func.bin func.asm
@REM D:\UTILS\ndisasm.exe func.bin > func.S

@C:\TDM-GCC-64\bin\strip.exe --strip-all test.exe
@D:\UTILS\upx.exe -q -9 test.exe
