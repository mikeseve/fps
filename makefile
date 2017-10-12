main: main.o FPS.o
	g++ -Wall -g -l wiringPi -l mosquitto  -o main main.o FPS.o
main.o: main.cpp FPS.h
	g++ -Wall -Wno-write-strings -g -l wiringPi -c main.cpp
FPS.o: FPS.cpp FPS.h
	g++ -Wall -g -l wiringPi -c FPS.cpp
clean:
	rm main *.o
