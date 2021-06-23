PORTSF_LIBS=-L../portsf/portsf -I../portsf/include -lportsf
FLAGS=-Wall -Wextra -g -lm $(PORTSF_LIBS)

build: main.c
	gcc main.c $(FLAGS) -o bin/audio-gain 