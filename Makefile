
project : main.cpp images.cpp ledstrip.cpp
	g++ -O2 main.cpp images.cpp ledstrip.cpp -o project `pkg-config --libs --cflags opencv`

clean :
	rm -f project *.o
