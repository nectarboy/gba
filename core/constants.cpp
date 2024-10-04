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

#define PRINTDEBUG		false						// Enables print debugging
#define PRINT_THUMB		true						// Enables prints in THUMB mode
#define PRINT_ARM		true						// Enables prints in ARM mode
#define PRINTPC_START	0x00						// The minimum PC should be when printing debug statements
#define PRINTPC_END		0x3ffffffff						// The maximum PC should be when printing debug statements ; BIOS ends at 3fff
#define PRINTPC_EXE		0x54bc1 - 100						// Minimum instructions ran before printing debug statements

#define TESTROMPATH "./roms/fuzzarm/ARM_Any.gba" // There is a bug present in THUMB_any
//#define TESTROMPATH "./roms/toncers/irq_demo.gba" // FIXME: WHY is this rom executing an invalid thumb instruction eb00 at 34a ?? Also, theres a top orange line thats missing
//#define TESTROMPATH "./roms/retAddr.gba"
//#define TESTROMPATH "./roms/armwrestler-gba-fixed.gba"
//#define TESTROMPATH "./roms/kirby.gba"

#define BIOSPATH "./roms/bios.bin"

#define CPI 1