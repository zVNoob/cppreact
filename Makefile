CFLAGS = -g -std=c++20 -MMD -I.

test: tests/test
	./tests/test

tests/%: tests/%.cpp
	g++ -o $@ $< $(shell sdl2-config --cflags --libs) $(shell pkg-config --cflags --libs freetype2 harfbuzz fontconfig SDL2_image) $(CFLAGS)

examples/%: examples/%.cpp
	g++ -o $@ $< $(shell sdl2-config --cflags --libs) $(shell pkg-config --cflags --libs freetype2 harfbuzz fontconfig SDL2_image) $(CFLAGS)

dashboard: examples/dashboard
	./examples/dashboard

counter: examples/counter
	./examples/counter

gallery: examples/gallery
	./examples/gallery

docs:
	doxygen

.PHONY: test docs dashboard counter gallery

-include tests/*.d
-include examples/*.d
