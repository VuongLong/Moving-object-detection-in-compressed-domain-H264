LIBS_OPENCV = `$(PREFIX)pkg-config --libs opencv`
INCLUDE_OPENCV = `$(PREFIX)pkg-config --cflags opencv`

default: 
	gcc -std=c99 -O3 -Wall -c vibe-background-sequential.c
	g++ -o main -O3 -Wall -pedantic $(INCLUDE_OPENCV) main.cpp vibe-background-sequential.o $(LIBS_OPENCV) 
