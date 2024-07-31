struct Core;

//10000 - User mode
//10001 - FIQ mode
//10010 - IRQ mode
//10011 - Supervisor mode
//10111 - Abort mode
//11011 - Undefined mode
//11111 - System mode
int getBankIDFromMode(int mode) {
	switch (mode) {
	case MODE_USER: return 0;
	case MODE_SYSTEM: return 0;
	case MODE_FIQ: return 1;
	case MODE_IRQ: return 2;
	case MODE_SVC: return 3;
	case MODE_ABT: return 4;
	case MODE_UND: return 5;
	default: std::cout << "[!] UNDEFINED MODE " << mode << "\n";
	}
}

struct Arm7 {
	// Component variables
	Core* core;
	Arm7(Core* _core) : core(_core) {}

	int test = 42;

	// General Registers
	u32 reg[16];
	inline u32 readReg(uint n) {
		return reg[n] + 8*(n == 15);
	}
	inline u64 writeReg(uint n, u32 val) {
		//if (n == 15) {
		//	if (cpsr.thumbMode)
		//		reg[n] = val & 0xffff'fffe; // Aligned to 2 bytes
		//	else
		//		reg[n] = val & 0xffff'fffc; // Aligned to 4 bytes 
		//}
		//else
		//	reg[n] = val;

		u32 alignmask = 0xffff'ffff << ((n == 15) * (2 - cpsr.thumbMode));
		reg[n] = val & alignmask;

		return val;
	}
	inline u64 writeRegBottomByte(uint n, u8 val) {
		return writeReg(n, (reg[n] & 0xffff'ff00) | val);
	}
	inline u64 writeRegBottomHalfword(uint n, u16 val) {
		return writeReg(n, (reg[n] & 0xffff'0000) | val);
	}

	// CPSR (Current Program Status Register)
	struct {
		unsigned int mode : 5;
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
		return (cpsr.mode) | (cpsr.thumbMode << 5) | (cpsr.FIQDisabled << 6) | (cpsr.IRQDisabled << 7) | (cpsr.flagV << 28) | (cpsr.flagC << 29) | (cpsr.flagZ << 30) | (cpsr.flagN << 31);
	}
	void writeCPSR(u32 val) {
		setMode(val & 0b11111);
		cpsr.thumbMode		= (val >> 5) & 1;
		cpsr.FIQDisabled	= (val >> 6) & 1;
		cpsr.IRQDisabled	= (val >> 7) & 1;
		cpsr.flagV			= (val >> 28) & 1;
		cpsr.flagC			= (val >> 29) & 1;
		cpsr.flagZ			= (val >> 30) & 1;
		cpsr.flagN			= (val >> 31) & 1;
	}

	// Banked Registers
	u32 bankedReg[6][7];
	u32 bankedSpsr[6];

	// Modes and Interrupts
	void setMode(int mode) {
		int oldmode = cpsr.mode;
		cpsr.mode = mode;

		int oldBankId = getBankIDFromMode(oldmode);
		int bankId = getBankIDFromMode(mode);
		
		if (oldBankId == bankId)
			return;

		// Bank switches for entering or exiting out of FIQ mode
		if (bankId == getBankIDFromMode(MODE_FIQ) || oldBankId == getBankIDFromMode(MODE_FIQ)) {
			for (int i = 0; i < 5; i++) {
				bankedReg[oldBankId][i] = reg[8 + i];
				reg[8 + i] = bankedReg[bankId][i];
			}
		}
		else if (bankId == getBankIDFromMode(MODE_IRQ)) {
			cpsr.IRQDisabled = true; //"On the GBA this is set by default whenever IRQ mode is entered. Why or how this is the case, I do not know."
		}

		if (bankId != 0) {
			copyCPSRToSPSR();
		}

		// Applies to all other banks
		bankedReg[oldBankId][13 - 8] = reg[13];
		bankedReg[oldBankId][14 - 8] = reg[14];
		reg[13] = bankedReg[bankId][13 - 8];
		reg[14] = bankedReg[bankId][14 - 8];
	}
	inline void copyCPSRToSPSR() {
		int bankId = getBankIDFromMode(cpsr.mode);
		bankedSpsr[bankId] = readCPSR();
		std::cout << "copying CPSR to SPSR " << bankId << "\n";
	}
	inline void copySPSRToCPSR() {
		int bankId = getBankIDFromMode(cpsr.mode);
		writeCPSR(bankedSpsr[bankId]);
		std::cout << "copying SPSR " << bankId << " to CPSR \n";
	}

	void setThumbMode(bool thumbMode) {
		cpsr.thumbMode = thumbMode;
	}

	// Reading and Writing to Memory
	u32 openBusVal;
	u32 read8(u32 addr) {
		return 0;
	}
	u32 readSigned8(u32 addr) {
		u32 val = read8(addr);
		return val | (0xffff'ff00 * (val >> 7));
	}
	u32 read8OptionalSign(u32 addr, bool signextend) {
		u32 val = read8(addr);
		return val | (0xffff'ff00 * (val >> 7) * signextend);
	}
	u32 read16(u32 addr) {
		return 0;
	}
	u32 read16OptionalSign(u32 addr, bool signextend) {
		u32 val = read16(addr);
		return val | (0xffff'0000 * (val >> 15) * signextend);
	}
	u32 readSigned16(u32 addr) {
		u32 val = read16(addr);
		return val | (0xffff'0000 * (val >> 15));
	}
	u32 read32(u32 addr) {
		return 0;
	}
	void write8(u32 addr, u8 val) {

	}
	void write16(u32 addr, u16 val) {

	}
	void write32(u32 addr, u32 val) {

	}

	// Execution

	// Initialization
	void init() {
		//std::cout << core->test << "\n";

		cpsr.mode = MODE_USER;

		// Zeroeing arrays
		for (int i = 0; i < lenOfArray(reg); i++)
			reg[i] = 0;
		for (int i = 0; i < lenOfArray(bankedReg); i++)
			for (int ii = 0; ii < lenOfArray(bankedReg[i]); ii++)
				bankedReg[i][ii] = 0;

		setMode(MODE_USER);
	}
};

#include "core/cpu/interpreter/arm32.cpp"
#include "core/cpu/interpreter/thumb16.cpp"