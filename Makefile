
project : main.cpp images.cpp ledstrip.cpp camera.cpp comm.c
	g++ -O3 main.cpp images.cpp ledstrip.cpp camera.cpp comm.c -o project `pkg-config --libs --cflags opencv`

clean :
	rm -f project *.o
