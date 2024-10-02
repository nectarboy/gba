#define EMU_NAME "Gerber Baby Advance"

#define SW 240
#define SH 160

#define MODE_USER		0b10000
#define MODE_SYSTEM		0b11111
#define MODE_FIQ		0b10001
#define MODE_IRQ		0b10010
#define MODE_SVC		0b10011
#define MODE_ABT		0b10111
#define MODE_UND		0b11011

#define PRINTDEBUG false					// Enables print debugging
#define PRINTPC 0							// The minimum PC should be when printing debug statements
#define PRINTEXE 0							// The minimum executions ran before printing debug statements

//#define TESTROMPATH "./roms/fuzzarm/THUMB_Any.gba" // There is a bug present
#define TESTROMPATH "./roms/toncers/irq_demo.gba"
#define TESTROMPATH "./roms/retAddr.gba"

#define BIOSPATH "./roms/bios.bin"