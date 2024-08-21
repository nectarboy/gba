#include "core/cpu/interpreter/conditioncodes.cpp"
#include "core/cpu/interpreter/arm32.cpp"
#include "core/cpu/interpreter/thumb16.cpp"

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
	default: return -1;
	}
}
std::string getModeStringFromMode(int mode) {
	switch (mode) {
	case MODE_USER: return "USER";
	case MODE_SYSTEM: return "SYS";
	case MODE_FIQ: return "FIQ";
	case MODE_IRQ: return "IRQ";
	case MODE_SVC: return "SVC";
	case MODE_ABT: return "ABT";
	case MODE_UND: return "UND";
	default: return "[INVALID MODE]";
	}
}

inline u32 Arm7::readReg(uint n) {
	return reg[n]; //+ 8*(n == 15); // TODO: remove this and fix the tests that break
}
inline u64 Arm7::writeReg(uint n, u32 val) {
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
inline u32 Arm7::readUserBankReg(uint n) {
	if (cpsr.mode == MODE_FIQ) {
		if (n >= 8 && n <= 14)
			return bankedReg[0][n - 8];
		else
			return reg[n];
	}
	else {
		if (n >= 13 && n <= 14)
			return bankedReg[0][n - 8];
		else
			return reg[n];
	}
}
inline u32 Arm7::writeUserBankReg(uint n, u32 val) {
	if (cpsr.mode == MODE_FIQ) {
		if (n >= 8 && n <= 14)
			bankedReg[0][n - 8] = val;
		else
			writeReg(n, val);
	}
	else {
		if (n >= 13 && n <= 14)
			bankedReg[0][n - 8] = val;
		else
			writeReg(n, val);
	}
	return val;
}

u32 Arm7::readCPSR() {
	return (cpsr.mode) | (cpsr.thumbMode << 5) | (cpsr.FIQDisabled << 6) | (cpsr.IRQDisabled << 7) | (cpsr.flagV << 28) | (cpsr.flagC << 29) | (cpsr.flagZ << 30) | (cpsr.flagN << 31);
}
void Arm7::writeCPSR(u32 val) {
	setMode(val & 0b11111);
	setThumbMode((val >> 5) & 1);
	cpsr.FIQDisabled	= (val >> 6) & 1;
	cpsr.IRQDisabled	= (val >> 7) & 1;
	cpsr.flagV			= (val >> 28) & 1;
	cpsr.flagC			= (val >> 29) & 1;
	cpsr.flagZ			= (val >> 30) & 1;
	cpsr.flagN			= (val >> 31) & 1;
}

inline u32 Arm7::readCurrentSPSR() {
	int bankId = getBankIDFromMode(cpsr.mode);
	if (bankId == 0)
		return readCPSR();
	else
		return bankedSpsr[bankId];
}
inline void Arm7::writeCurrentSPSR(u32 val) {
	int bankId = getBankIDFromMode(cpsr.mode);
	if (bankId != 0)
		bankedSpsr[bankId] = val & 0xf000'00ff; // TODO: remove this later, debug MSR if it causes issues
}

// Modes and Interrupts
void Arm7::setMode(int mode) {
	int oldmode = cpsr.mode;
	if (oldmode != mode)
		std::cout << "Setting mode,\t" << getModeStringFromMode(oldmode) << " --> " << getModeStringFromMode(mode) << "\n";

	int oldBankId = getBankIDFromMode(oldmode);
	int bankId = getBankIDFromMode(mode);
	if (bankId == -1) {
		std::cout << "[!] INVALID MODE " << std::bitset<5>(mode) << "\n";
		PRINTSTATE();
	}

	cpsr.mode = mode;
		
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

	// Applies to all other banks
	bankedReg[oldBankId][13 - 8] = reg[13];
	bankedReg[oldBankId][14 - 8] = reg[14];
	reg[13] = bankedReg[bankId][13 - 8];
	reg[14] = bankedReg[bankId][14 - 8];
}
inline void Arm7::copyCPSRToSPSR() {
	int bankId = getBankIDFromMode(cpsr.mode);
	if (bankId != 0) {
		std::cout << "copying CPSR to SPSR " << getModeStringFromMode(cpsr.mode) << " (bank " << bankId << ") \n";
		bankedSpsr[bankId] = readCPSR();
	}
}
inline void Arm7::copySPSRToCPSR() {
	int bankId = getBankIDFromMode(cpsr.mode);
	if (bankId != 0) {
		std::cout << "copying SPSR " << getModeStringFromMode(cpsr.mode) << " (bank " << bankId << ") to CPSR \n";
		writeCPSR(bankedSpsr[bankId]);
	}
}

void Arm7::setThumbMode(bool thumbMode) {
	bool swapped = thumbMode ^ cpsr.thumbMode;
	cpsr.thumbMode = thumbMode;

	if (swapped) {
		if (thumbMode) {
			print("Switched to THUMB");
			//PRINTSTATE();
		}
		else {
			print("Switched to ARM");
		}
	}
}

// Reading and Writing to Memory
int vblank_stub = 0;
u32 vblanktimer = 0;
u32 Arm7::read8(u32 addr) {
	// if (canPrint()) std::cout << "read:\t" << std::hex << addr << std::dec << "\n";
	if (addr < 0x0000'4000) {
		return core->mem->bios[addr];
	}
	if (addr >= 0x0200'0000 && addr < 0x0204'0000) {
		return core->mem->wramb[addr - 0x0200'0000];
	}
	if (addr >= 0x0300'0000 && addr < 0x0300'8000) {
		return core->mem->wramc[addr - 0x0300'0000];
	}
	if (addr >= 0x0400'0000 && addr < 0x040003FE) {
		if (addr == 0x0400'0004) {
			if (vblanktimer++ >= 10) {
				vblank_stub ^= 1;
				vblanktimer = 0;
			}
			//print("READ DISPSTAT");
			return vblank_stub;
		}
		if ((addr & 0xffff'fffe) == 0x0400'0130) {
			//print("READ KEYINPUT");
			return 0xff;
		}
	}
	if (addr >= 0x0500'0000 && addr < 0x0500'0400) {
		if (canPrint()) std::cout << "PALLETE read:\t" << std::hex << addr << std::dec << "\n";
		return core->mem->palleteram[addr - 0x0500'0000];
	}
	if (addr >= 0x0600'0000 && addr < 0x0601'8000) {
		if (canPrint()) std::cout << "VRAM read:\t" << std::hex << addr << std::dec << "\n";
		return core->mem->vram[addr - 0x0600'0000];
	}
	if (addr >= 0x0800'0000 && addr < 0x0e01'0000) {
		addr -= 0x0800'0000;
		if (addr >= core->mem->romSize) {
			test = 0;
			//std::cout << "Address exceeds rom size:\t" << std::hex << addr + 0x0800'0000 << " " << reg[15] << std::dec << "\n";
			return 0;
		}
		return core->mem->rom[addr];
	}
	return 0;
}
u32 Arm7::readSigned8(u32 addr) {
	u32 val = read8(addr);
	return val | (0xffff'ff00 * (val >> 7));
}
u32 Arm7::read16(u32 addr) {
	return read8(addr++) | (read8(addr) << 8);
	return 0;
}
u32 Arm7::readSigned16(u32 addr) {
	u32 val = read16(addr);
	return val | (0xffff'0000 * (val >> 15));
}
u32 Arm7::read32(u32 addr) {
	return read8(addr++) | (read8(addr++) << 8) | (read8(addr++) << 16) | (read8(addr) << 24);
	return 0;
}
void Arm7::write8(u32 addr, u8 val) {
	if (addr >= 0x0200'0000 && addr < 0x0204'0000) {
		if (canPrint()) std::cout << "WRAMB write:\t" << std::hex << addr << ", " << u32(val) << std::dec << "\n";
		core->mem->wramb[addr - 0x0200'0000] = val;
	}
	if (addr >= 0x0300'0000 && addr < 0x0300'8000) {
		if (canPrint()) std::cout << "WRAMC write:\t" << std::hex << addr << ", " << u32(val) << std::dec << "\n";
		core->mem->wramc[addr - 0x0300'0000] = val;
	}
	if (addr >= 0x04000000 && addr < 0x040003FE) {
		if (canPrint()) std::cout << "IO write:\t" << std::hex << addr << ", val:\t" << u32(val) << std::dec << "\n";
	}
	if (addr >= 0x0500'0000 && addr < 0x0500'0400) {
		if (canPrint()) std::cout << "PALLETE write:\t" << std::hex << addr << ", val:\t" << u32(val) << std::dec << "\n";
		core->mem->palleteram[addr - 0x0500'0000] = val;
	}
	if (addr >= 0x0600'0000 && addr < 0x0601'8000) {
		if (canPrint()) std::cout << "VRAM write:\t" << std::hex << addr << ", val:\t" << u32(val) << std::dec << "\n";
		core->mem->vram[addr - 0x0600'0000] = val;
	}
	else {
		if (canPrint()) std::cout << "Write:\t" << std::hex << addr << ", val:\t" << u32(val) << std::dec << "\n";
	}
}
void Arm7::write16(u32 addr, u16 val) {
	write8(addr++, val >> 0);
	write8(addr, val >> 8);
}
void Arm7::write32(u32 addr, u32 val) {
	write8(addr++, val >> 0);
	write8(addr++, val >> 8);
	write8(addr++, val >> 16);
	write8(addr, val >> 24);
}

// Execution
void Arm7::checkForInterrupts() {

}
void Arm7::execute() {
	_lastPC = reg[15];

	checkForInterrupts();
	if (!cpsr.thumbMode) {
		u32 instruction = Arm32_FetchInstruction(this);
		ArmInstructionFunc func = Arm32_Decode(this, instruction);
		func(this, instruction);
	}
	else {
		u16 instruction = Thumb16_FetchInstruction(this);
		ThumbInstructionFunc func = Thumb16_Decode(this, instruction);
		func(this, instruction);
	}

	_executionsRan++;
}

// Initialization
void Arm7::bootstrap() {
	reg[0] = 0xca5;
	reg[15] = 0x0800'0000;
	reg[13] = 0x0300'7f00;
	writeCPSR(0x0000'001f); // 0x6000'001f
}
void Arm7::reset() {
	//std::cout << "Core's test value: " << core->test << "\n";
	//std::cout << "Mem's test value: " << core->mem->test << "\n";

	// Zeroeing arrays
	for (int i = 0; i < lenOfArray(reg); i++)
		reg[i] = 0;
	for (int i = 0; i < lenOfArray(bankedReg); i++)
		for (int ii = 0; ii < lenOfArray(bankedReg[i]); ii++)
			bankedReg[i][ii] = 0;

	cpsr.mode = MODE_USER;
	setMode(MODE_USER);

	bootstrap();
}

 // Debugging
void Arm7::PRINTSTATE() {
	std::cout << std::hex << "\nr0: " << reg[0] << "\nr1: " << reg[1] << "\nr2: " << reg[2] << "\nr3: " << reg[3] << "\nr4: " << reg[4] << "\nr5: " << reg[5] << "\nr6: " << reg[6] << "\nr7: " << reg[7] << "\nr8: " << reg[8] << "\nr9: " << reg[9] << "\nr10: " << reg[10] << "\nr11: " << reg[11] << "\nr12: " << reg[12] << "\nr13: " << reg[13] << "\nr14: " << reg[14] << "\nr15: " << reg[15] << "\nCPSR: " << readCPSR() << "\nSPSR: " << readCurrentSPSR() << "\n\n";
	assert(0);
}
void Arm7::BEFOREFETCH() {
	// BREAKPOINT
	if (reg[15] == 0x0800'0534) {
		print("BREAKPOINT");
		PRINTSTATE();
	}

	// OOB CHECK
	if (reg[15] == 0 || (reg[15] < 0x0800'0000 /*&& >reg[15] >= 0x0000'4000*/)) {
		print("OUT OF BOUNDS");
		PRINTSTATE();
	}
}
constexpr bool Arm7::canPrint() {
	return (PRINTDEBUG && _lastPC >= PRINTPC && _executionsRan >= PRINTEXE);
}