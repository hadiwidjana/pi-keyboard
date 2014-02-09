EXECS = retrogame
CC    = gcc -Wall -O3 -fomit-frame-pointer -funroll-loops -s -lwiringPi

all: $(EXECS)

retrogame: retrogame.c
	$(CC) $< -o $@
	strip $@
	
rotaryencoder: rotaryencoder.c
	$(CC) $< -o $@
	strip $@

clean:
	rm -f $(EXECS)
