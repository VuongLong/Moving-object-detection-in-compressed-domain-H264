LIBS_OPENCV = `$(PREFIX)pkg-config --libs opencv`
INCLUDE_OPENCV = `$(PREFIX)pkg-config --cflags opencv`

default: 
	gcc -std=c99 -O3  -c vibe-background-sequential.c
	g++ -o main -O3 -Wall -Werror -pedantic $(INCLUDE_OPENCV) s
