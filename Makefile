EXECS = retrogame
CC	= gcc -Wall -O3 -fomit-frame-pointer -funroll-loops -s -lwiringPi
LDLIBS	= -lwiringPi
SRC	= retrogame.c rotaryencoder.c


retrogame: $(SRC)
	$(CC) $< -o $(EXECS) $(SRC)

clean:
	rm -f $(EXECS)
