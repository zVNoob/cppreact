
all:
	g++ test.cpp -o test -g -lX11 -lXext

linux:
	g++ test.cpp -o test -g -lX11 -lXext

windows:
	x86_64-w64-mingw32-g++ test.cpp -o test -g -mwindows -static
