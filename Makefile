
all:
	g++ test.cpp -o test -g -lX11 -D DEBUG

linux:
	g++ test.cpp -o test -g -lX11

windows:
	x86_64-w64-mingw32-g++ test.cpp -o test -g -static -mconsole -mwindows
