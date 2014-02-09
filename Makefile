EXECS	= retrogame
CC	= gcc -Wall -O3 -fomit-frame-pointer -funroll-loops -s 
LDLIBS	= -lwiringPi
SRC	= retrogame.c rotaryencoder.c rotaryencoder.h

all: $(EXECS)

retrogame: retrogame.c
	$(CC) $< -o $@ $(LDLIBS)
	strip $@
	
rotaryencoder: rotaryencoder.c
	$(CC) $< -o $@ $(LDLIBS)
	

clean:
	rm -f $(EXECS)
