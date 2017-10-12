main: main.o FPS.o MQTT.o
	g++ -Wall -g -l wiringPi -l mosquitto  -o main main.o FPS.o MQTT.o
main.o: main.cpp FPS.h
	g++ -Wall -Wno-write-strings -g -l wiringPi -c main.cpp
MQTT.o: MQTT.cpp MQTT.h
	g++ -Wall -g -l mosquitto -c MQTT.cpp
FPS.o: FPS.cpp FPS.h
	g++ -Wall -g -l wiringPi -c FPS.cpp
clean:
	rm main *.o
