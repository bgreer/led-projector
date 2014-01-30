
project : main.cpp images.cpp ledstrip.cpp camera.cpp
	g++ -O2 main.cpp images.cpp ledstrip.cpp camera.cpp -o project `pkg-config --libs --cflags opencv`

clean :
	rm -f project *.o
