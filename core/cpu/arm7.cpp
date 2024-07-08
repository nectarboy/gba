#include <iostream>
#include "helpers.h"
#include "core/cpu/interpreter/arm32.cpp"
#include "core/cpu/interpreter/thumb16.cpp"

//10000 - User mode
//10001 - FIQ mode
//10010 - IRQ mode
//10011 - Supervisor mode
//10111 - Abort mode
//11011 - Undefined mode
//11111 - System mode
enum ProcessorMode {
	USER = 0b10000,
	FIQ = 0b10001,
	IRQ = 0b10010,
	SVC = 0b10011,
	ABT = 0b10111,
	UND = 0b11011,
	SYSTEM = 0b11111
};
int getBankIDFromMode(ProcessorMode mode) {
	switch (mode) {
	case USER: return 0;
	case SYSTEM: return 0;
	case FIQ: return 1;
	case IRQ: return 2;
	case SVC: return 3;
	case ABT: return 4;
	case UND: return 5;
	}
}

struct Arm7 {
	// Component variables
	// gba
	// mem

	// General Registers
	u32 reg[16];

	// CPSR (Current Program Status Register)
	struct {
		ProcessorMode mode : 5;
		bool thumbMode : 1;
		bool FIQDisabled : 1;
		bool IRQDisabled : 1;
		unsigned int : 19; // Reserved (8-27)
		bool flagV : 1;
		bool flagC : 1;
		bool flagZ : 1;
		bool flagN : 1;

		u32 read() {
			return (mode) | (thumbMode << 5) | (FIQDisabled << 6) | (IRQDisabled << 7) | (flagV << 28) | (flagC << 29) | (flagZ << 30) | (flagZ << 31);
		}
		//void write(u32 val) {
		//	mode = static_cast<ProcessorMode>(val & 0b11111);
		//	thumb
		//}
	} cpsr;

	// Banked Registers
	u32 bankedReg[6][7];
	u32 bankedSvsr[5];

	void setMode(ProcessorMode mode) {
		ProcessorMode oldmode = cpsr.mode;

		int oldBankId = getBankIDFromMode(oldmode);
		int bankId = getBankIDFromMode(mode);

		if (oldBankId == bankId)
			return;

		// Entering or exiting out of FIQ mode
		if (bankId == getBankIDFromMode(FIQ) || oldBankId == getBankIDFromMode(FIQ)) {
			for (int i = 0; i < 5; i++) {
				bankedReg[oldBankId][i] = reg[8 + i];
				reg[8 + i] = bankedReg[bankId][i];
			}
		}

		// Applies to all other banks
		bankedReg[oldBankId][13 - 8] = reg[13];
		bankedReg[oldBankId][14 - 8] = reg[14];
		reg[13] = bankedReg[bankId][13 - 8];
		reg[14] = bankedReg[bankId][14 - 8];

	}

	// Initialization
	void init() {
		setMode(USER);
		std::fill(std::begin(reg), std::end(reg), 0);
		for (int i = 0; i < lenOfArray(bankedReg); i++)
			std::fill(std::begin(bankedReg[i]), std::end(bankedReg[i]), 0);
	}
};