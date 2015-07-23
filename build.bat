@REM without -O3 optimization, see stack arguments
@C:\TDM-GCC-64\bin\gcc.exe -std=c99 -c -o test.obj test.c
@C:\TDM-GCC-64\bin\gcc.exe -std=c99 -c -o sbt.obj sbt.c

@C:\TDM-GCC-64\bin\gcc.exe test.obj sbt.obj -o test.exe
@C:\TDM-GCC-64\bin\strip.exe --strip-all test.exe
@D:\UTILS\upx.exe -q -9 test.exe
