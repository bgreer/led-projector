
FLAGS=-O3

project : main.cpp images.cpp ledstrip.cpp camera.cpp comm.c sequencer.cpp effects_new.cpp parse_params.c
	g++ ${FLAGS} main.cpp images.cpp ledstrip.cpp camera.cpp comm.c sequencer.cpp effects_new.cpp parse_params.c -o project `pkg-config --libs --cflags opencv`

clean :
	rm -f project *.o
