#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/mman.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <wiringPi.h>
#include "rotaryencoder.h"


// START HERE ------------------------------------------------------------
// This table remaps GPIO inputs to keyboard values.  In this initial
// implementation there's a 1:1 relationship (can't attach multiple keys
// to a button) and the list is fixed in code; there is no configuration
// file.  Buttons physically connect between GPIO pins and ground.  There
// are only a few GND pins on the GPIO header, so a breakout board is
// often needed.  If you require just a couple extra ground connections
// and have unused GPIO pins, set the corresponding key value to GND to
// create a spare ground point.

#define GND -1


struct {
	int pin;
	int key;
} io[] = {
//	  Input    Output (from /usr/include/linux/input.h)
	{ 2,      KEY_UP     },
	{ 3,      KEY_DOWN     },
	{ 4,      KEY_LEFT     },
	{ 17,     KEY_RIGHT     },
	{ 18,      KEY_SPACE     },
	{ 27,      KEY_BACKSPACE     },
	{ 22,      KEY_ENTER     },
	{ 23,      193     },
	{ 24,	194     },

};


#define IOLEN (sizeof(io) / sizeof(io[0])) // io[] table size





// A few globals ---------------------------------------------------------

char
  *progName,                         // Program name (for error reporting)
   sysfs_root[] = "/sys/class/gpio", // Location of Sysfs GPIO files
   running      = 1;                 // Signal handler will set to 0 (exit)
volatile unsigned int
  *gpio;                             // GPIO register table


// Some utility functions ------------------------------------------------

// Set one GPIO pin attribute through the Sysfs interface.
int pinConfig(int pin, char *attr, char *value) {
	char filename[50];
	int  fd, w, len = strlen(value);
	sprintf(filename, "%s/gpio%d/%s", sysfs_root, pin, attr);
	if((fd = open(filename, O_WRONLY)) < 0) return -1;
	w = write(fd, value, len);
	close(fd);
	return (w != len); // 0 = success
}

// Un-export any Sysfs pins used; don't leave filesystem cruft.  Also
// restores any GND pins to inputs.  Write errors are ignored as pins
// may be in a partially-initialized state.
void cleanup() {
	char buf[50];
	int  fd, i;
	sprintf(buf, "%s/unexport", sysfs_root);
	if((fd = open(buf, O_WRONLY)) >= 0) {
		for(i=0; i<IOLEN; i++) {
			// Restore GND items to inputs
			if(io[i].key == GND)
				pinConfig(io[i].pin, "direction", "in");
			// And un-export all items regardless
			sprintf(buf, "%d", io[i].pin);
			write(fd, buf, strlen(buf));
		}
		close(fd);
	}
}

// Quick-n-dirty error reporter; print message, clean up and exit.
void err(char *msg) {
	printf("%s: %s.  Try 'sudo %s'.\n", progName, msg, progName);
	cleanup();
	exit(-1);
}

// Interrupt handler -- set global flag to abort main loop.
void signalHandler(int n) {
	running = 0;
}


// Main stuff ------------------------------------------------------------

#define BCM2708_PERI_BASE 0x20000000
#define GPIO_BASE         (BCM2708_PERI_BASE + 0x200000)
#define BLOCK_SIZE        (4*1024)
#define GPPUD             (0x94 / 4)
#define GPPUDCLK0         (0x98 / 4)

int main(int argc, char *argv[]) {

	// A few arrays here are declared with IOLEN elements, even though
	// values aren't needed for io[] members where the 'key' value is
	// GND.  This simplifies the code a bit -- no need for mallocs and
	// tests to create these arrays -- but may waste a handful of
	// bytes for any declared GNDs.
	
	int keyboard[] = { 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8, 0, 9, 0, 10, 0, 11, 0, 30, 0, 
			48, 0, 46, 0, 32, 0, 18, 0, 33, 0, 34, 0, 35, 0, 23, 0, 36, 0,
			37, 0, 38, 0, 50, 0, 49, 0, 24, 0, 25, 0, 16, 0, 19, 0, 31, 0, 
			20, 0, 22, 0, 47, 0, 17, 0, 45, 0, 21, 0, 44};
	
	
	int b = 0;
	char                   buf[50],         // For sundry filenames
	                       c;               // Pin input value ('0'/'1')
	int                    fd,              // For mmap, sysfs, uinput
	                       i, j,            // Asst. counter
	                       bitmask,         // Pullup enable bitmask
	                       timeout = -1,    // poll() timeout
	                       intstate[IOLEN], // Last-read state
	                       extstate[IOLEN]; // Debounced state
	volatile unsigned char shortWait;       // Delay counter
	struct uinput_user_dev uidev;           // uinput device
	struct input_event     keyEv, synEv;    // uinput events
	struct pollfd          p[IOLEN];        // GPIO file descriptors

	progName = argv[0];             // For error reporting
	signal(SIGINT , signalHandler); // Trap basic signals (exit cleanly)
	signal(SIGKILL, signalHandler);


	// ----------------------------------------------------------------
	// Although Sysfs provides solid GPIO interrupt handling, there's
	// no interface to the internal pull-up resistors (this is by
	// design, being a hardware-dependent feature).  It's necessary to
	// grapple with the GPIO configuration registers directly to enable
	// the pull-ups.  Based on GPIO example code by Dom and Gert van
	// Loo on elinux.org
	
	int keypress = 30;	//store the keyboard value from input.h
	int rotate;	//the value from rotary encoder
	int oldrotate = 0;	//
	

	
	
	
	
	if((fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0)
		err("Can't open /dev/mem");
	gpio = mmap(            // Memory-mapped I/O
	  NULL,                 // Any adddress will do
	  BLOCK_SIZE,           // Mapped block length
	  PROT_READ|PROT_WRITE, // Enable read+write
	  MAP_SHARED,           // Shared with other processes
	  fd,                   // File to map
	  GPIO_BASE );          // Offset to GPIO registers
	close(fd);              // Not needed after mmap()
	if(gpio == MAP_FAILED) err("Can't mmap()");
	// Make combined bitmap of pullup-enabled pins:
	for(bitmask=i=0; i<IOLEN; i++)
		if(io[i].key != GND) bitmask |= (1 << io[i].pin);
	gpio[GPPUD]     = 2;                    // Enable pullup
	for(shortWait=150;--shortWait;);        // Min 150 cycle wait
	gpio[GPPUDCLK0] = bitmask;              // Set pullup mask
	for(shortWait=150;--shortWait;);        // Wait again
	gpio[GPPUD]     = 0;                    // Reset pullup registers
	gpio[GPPUDCLK0] = 0;
	(void)munmap((void *)gpio, BLOCK_SIZE); // Done with GPIO mmap()




	

	// ----------------------------------------------------------------
	// All other GPIO config is handled through the sysfs interface.

	sprintf(buf, "%s/export", sysfs_root);
	if((fd = open(buf, O_WRONLY)) < 0) // Open Sysfs export file
		err("Can't open GPIO export file");
	for(i=j=0; i<IOLEN; i++) { // For each pin of interest...
		sprintf(buf, "%d", io[i].pin);
		write(fd, buf, strlen(buf));             // Export pin
		pinConfig(io[i].pin, "active_low", "0"); // Don't invert
	
		
		if(io[i].key == GND) {
			// Set pin to output, value 0 (ground)
			if(pinConfig(io[i].pin, "direction", "out") ||
			   pinConfig(io[i].pin, "value"    , "0"))
				err("Pin config failed (GND)");
		} else {
			// Set pin to input, detect rise+fall events
			if(pinConfig(io[i].pin, "direction", "in") ||
			   pinConfig(io[i].pin, "edge"     , "both"))
				err("Pin config failed");
			// Get initial pin value
			sprintf(buf, "%s/gpio%d/value",
			  sysfs_root, io[i].pin);
			// The p[] file descriptor array isn't necessarily
			// aligned with the io[] array.  GND keys in the
			// latter are skipped, but p[] requires contiguous
			// entries for poll().  So the pins to monitor are
			// at the head of p[], and there may be unused
			// elements at the end for each GND.  Same applies
			// to the intstate[] and extstate[] arrays.
			if((p[j].fd = open(buf, O_RDONLY)) < 0)
				err("Can't access pin value");
			intstate[j] = 0;
			if((read(p[j].fd, &c, 1) == 1) && (c == '0'))
				intstate[j] = 1;
			extstate[j] = intstate[j];
			p[j].events  = POLLPRI; // Set up poll() events
			p[j].revents = 0;
			j++;
		}
		
		
	} // 'j' is now count of non-GND items in io[] table
	close(fd); // Done exporting


	// ----------------------------------------------------------------
	// Set up uinput

	if((fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK)) < 0)
		err("Can't open /dev/uinput");
	if(ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0)
		err("Can't SET_EVBIT");
		
	for(i=0; i<70; i++) {
			if(ioctl(fd, UI_SET_KEYBIT, i) < 0)
				err("Can't SET_KEYBIT");
		
	}

	memset(&uidev, 0, sizeof(uidev));
	snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "retrogame");
	uidev.id.bustype = BUS_USB;
	uidev.id.vendor  = 0x1;
	uidev.id.product = 0x1;
	uidev.id.version = 1;
	if(write(fd, &uidev, sizeof(uidev)) < 0)
		err("write failed");
	if(ioctl(fd, UI_DEV_CREATE) < 0)
		err("DEV_CREATE failed");
	// Initialize input event structures
	memset(&keyEv, 0, sizeof(keyEv));
	keyEv.type  = EV_KEY;
	memset(&synEv, 0, sizeof(synEv));
	synEv.type  = EV_SYN;
	synEv.code  = SYN_REPORT;
	synEv.value = 0;
	
	// 'fd' is now open file descriptor for issuing uinput events


	// ----------------------------------------------------------------
	// Monitor GPIO file descriptors for button events.  The poll()
	// function watches for GPIO IRQs in this case; it is NOT
	// continually polling the pins!  Processor load is near zero.

	/*wiringPiSetupGpio () ;
	struct encoder *keyr = setupencoder(13, 12);
	rotate = keyr->value;*/
	

	
	while(running) { // Signal handler can set this to 0 to exit
		// Wait for IRQ on pin (or timeout for button debounce)
		
				
			
		/*	if (rotate < keyr->value) {
				keyEv.code  = KEY_BACKSPACE;
				keyEv.value = 1;
				write(fd, &keyEv, sizeof(keyEv));
				keyEv.code  = keypress+1;
				keyEv.value = 1;
				write(fd, &keyEv, sizeof(keyEv));
				c = 1;
				rotate = keyr->value;	
			} else if (rotate > keyr->value) {
				keyEv.code  = KEY_BACKSPACE;
				keyEv.value = 1;
				write(fd, &keyEv, sizeof(keyEv));
				keyEv.code  = keypress-1;
				keyEv.value = 1;
				write(fd, &keyEv, sizeof(keyEv));
				c = 1;
				rotate = keyr->value;	
			} */
			
		if(poll(p, j, timeout) > 0) { // If IRQ...
				
			
			for(i=0; i<j; i++) {       // Scan non-GND pins...
				if(p[i].revents) { // Event received?
					// Read current pin state, store
					// in internal state flag, but
					// don't issue to uinput yet --
					// must wait for debounce!
					b++;
					lseek(p[i].fd, 0, SEEK_SET);
					read(p[i].fd, &c, 1);
					if(c == '0')      intstate[i] = 1;
					else if(c == '1') intstate[i] = 0;
					p[i].revents = 0; // Clear flag
				}
			}
			timeout = 20; // Set timeout for debounce
		} else { // Else timeout occurred; input is debounced
			// 'j' (number of non-GNDs) is re-counted as
			// it's easier than maintaining an additional
			// remapping table or a duplicate key[] list.
			for(c=i=j=0; i<IOLEN; i++) {
				if(io[i].key != GND) {
					// Compare internal state against
					// previously-issued value.  Send
					// keystrokes only for changed states.
					if(intstate[j] != extstate[j]) {
						extstate[j] = intstate[j];
						//if (io[i].key == 193)
						//{
							
							
							keyEv.code  = 2++;
							keyEv.value = intstate[j];
							write(fd, &keyEv,
							  sizeof(keyEv));
						/*} else
						if (io[i].key == 194)
						{
							
							keyEv.code  = keyboard[b];
							b=b-1;
							
							if (b==-1)
							{b=35;}
							
							keyEv.value = intstate[j];
							write(fd, &keyEv,
							  sizeof(keyEv));
						} else {
						keyEv.code  = io[i].key;
						keyEv.value = intstate[j];
						write(fd, &keyEv,
						  sizeof(keyEv));
						}*/
						c = 1; // Follow w/SYN event
						/*b++;
							
							if (b>72)
							{b=0;}
						*/
						
					}
					
							
					j++;
				}
			}
			if(c) write(fd, &synEv, sizeof(synEv));
			timeout = -1; // Return to normal IRQ monitoring
			
		}
	

	
		
	
	
	}

	// ----------------------------------------------------------------
	// Clean up

	ioctl(fd, UI_DEV_DESTROY); // Destroy and
	close(fd);                 // close uinput
	cleanup();                 // Un-export pins

	puts("Done.");

	return 0;
}
