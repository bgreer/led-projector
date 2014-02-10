
project : main.cpp images.cpp ledstrip.cpp camera.cpp comm.c effects.cpp parse_params.c
	g++ -O3 main.cpp images.cpp ledstrip.cpp camera.cpp comm.c effects.cpp parse_params.c -o project `pkg-config --libs --cflags opencv`

clean :
	rm -f project *.o
