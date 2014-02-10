EXECS = retrogame
CC	= gcc -Wall -O3 -fomit-frame-pointer -funroll-loops -s 
LDLIBS	= -lwiringPi
SRC	= retrogame.c rotaryencoder.c rotaryencoder.h

retrogame: retrogame.c rotaryencoder.c
	$(CC) $< -o $(LDLIBS) $@
	strip $@

clean:
	rm -f $(EXECS)
