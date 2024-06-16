a.out: main.c 
	gcc -g -Wall -I. -I raylib/src main.c chute.c raylib/src/libraylib.a -lm -lpthread

