# Makefile for the mosquitto-test examples.
# Copyright (c)2016 Kevin Boone

all: test-pub test-sub

# Some or all of these optional libraries may need to be included, 
#   depending how the Mosquitto library was configured. Of course, it
#   the are needed, then the actual libraries must be installed as well.
# -lssl will be -lssl3 on some Linux systems.

#OPTIONAL_LIBS=-lcrypto -lssl -lcares
LIBS=-lmosquitto -lpthread
FLAGS=-Wall -s

test-pub: test-pub.c
	gcc $(FLAGS) -o test-pub test-pub.c $(LIBS) $(OPTIONAL_LIBS) 

test-sub: test-sub.c
	gcc $(FLAGS) -o test-sub test-sub.c $(LIBS) $(OPTIONAL_LIBS) 

clean:
	rm -f *.o test-pub test-sub
