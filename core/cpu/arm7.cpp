#include <iostream>
#include "inttypes.h"
#include "core/cpu/interpreter/arm32.cpp"
#include "core/cpu/interpreter/thumb16.cpp"

//10000 - User mode
//10001 - FIQ mode
//10010 - IRQ mode
//10011 - Supervisor mode
//10111 - Abort mode
//11011 - Undefined mode
//11111 - System mode
enum ProcessorModes {
	USER = 0b10000,
	FIQ = 0b10001,
	IRQ = 0b10010,
	SVC = 0b10011,
	ABT = 0b10111,
	UND = 0b11011,
	SYSTEM = 0b11111
};

struct Arm7 {
	// Component variables
	// gba
	// mem

	// General Registers
	u32 r[16];

	// CPSR (Current Program Status Register)
	struct {
		ProcessorModes mode : 5;
		bool thumbMode : 1;
		bool FIQDisabled : 1;
		bool IRQDisabled : 1;
		unsigned int : 19; // Reserved (8-27)
		bool flagV : 1;
		bool flagC : 1;
		bool flagZ : 1;
		bool flagN : 1;
	} cpsr;
	u32 readCPSR() {
		// bit shifting bullshit goes here
	}
	void writeCPSR(u32 val) {
		// ...
	}

	// Initialization
	void init() {
		std::fill(std::begin(r), std::end(r), 0);
		//std::fill(std::begin(bankedr), std::end(bankedr), 0);
	}
};