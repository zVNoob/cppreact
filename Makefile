all: linux
	

linux:
	g++ test.cpp -o test -g -lX11 -lXrender -lXft `pkg-config --cflags freetype2`

windows:
	x86_64-w64-mingw32-g++ test.cpp -o test -g -static -mconsole -mwindows

sfml:
	g++ test.cpp -o test -g -lsfml-graphics -lsfml-window -lsfml-system	

sdl:
	g++ test.cpp -o test -g -lSDL2 

