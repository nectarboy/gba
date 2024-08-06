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
	default: std::cout << "[!] UNDEFINED MODE " << mode << "\n";
	}
}

inline u32 Arm7::readReg(uint n) {
	return reg[n] + 8*(n == 15);
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
//inline u64 Arm7::writeRegBottomByte(uint n, u8 val) {
//	return writeReg(n, (reg[n] & 0xffff'ff00) | val);
//}
//inline u64 Arm7::writeRegBottomHalfword(uint n, u16 val) {
//	return writeReg(n, (reg[n] & 0xffff'0000) | val);
//}

u32 Arm7::readCPSR() {
	return (cpsr.mode) | (cpsr.thumbMode << 5) | (cpsr.FIQDisabled << 6) | (cpsr.IRQDisabled << 7) | (cpsr.flagV << 28) | (cpsr.flagC << 29) | (cpsr.flagZ << 30) | (cpsr.flagN << 31);
}
void Arm7::writeCPSR(u32 val) {
	setMode(val & 0b11111);
	cpsr.thumbMode		= (val >> 5) & 1;
	cpsr.FIQDisabled	= (val >> 6) & 1;
	cpsr.IRQDisabled	= (val >> 7) & 1;
	cpsr.flagV			= (val >> 28) & 1;
	cpsr.flagC			= (val >> 29) & 1;
	cpsr.flagZ			= (val >> 30) & 1;
	cpsr.flagN			= (val >> 31) & 1;
}

// Modes and Interrupts
void Arm7::setMode(int mode) {
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
inline void Arm7::copyCPSRToSPSR() {
	int bankId = getBankIDFromMode(cpsr.mode);
	bankedSpsr[bankId] = readCPSR();
	std::cout << "copying CPSR to SPSR " << bankId << "\n";
}
inline void Arm7::copySPSRToCPSR() {
	int bankId = getBankIDFromMode(cpsr.mode);
	writeCPSR(bankedSpsr[bankId]);
	std::cout << "copying SPSR " << bankId << " to CPSR \n";
}

void Arm7::setThumbMode(bool thumbMode) {
	cpsr.thumbMode = thumbMode;
}

// Reading and Writing to Memory
int vblank_stub = 0;
u32 Arm7::read8(u32 addr) {
	if (canPrint()) std::cout << "read:\t" << std::hex << addr << std::dec << "\n";
	if (addr >= 0x0200'0000 && addr < 0x0204'0000) {
		return core->mem->wramb[addr - 0x0200'0000];
	}
	if (addr >= 0x0300'0000 && addr < 0x0300'8000) {
		return core->mem->wramc[addr - 0x0300'0000];
	}
	if (addr >= 0x0400'0000 && addr < 0x0600'0000) {
		if (addr == 0x0400'0004) {
			vblank_stub ^= 1;
			return vblank_stub;
		}
	}
	if (addr >= 0x0600'0000 && addr < 0x0601'8000) {
		//std::cout << "VRAM read:\t" << std::hex << addr << std::dec << "\n";
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
	checkForInterrupts();
	_lastPC = reg[15];
	u32 instruction = Arm32_FetchInstruction(this);
	InstructionFunction func = Arm32_Decode(this, instruction);
	func(this, instruction);
	_executionsRan++;
}

// Initialization
void Arm7::bootstrap() {
	reg[0] = 0xca5;
	reg[15] = 0x0800'0000;
	reg[13] = 0x0300'7f00; // wouldve been nice to know before implementing ldmstm
	writeCPSR(0x6000'001f);
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
bool Arm7::canPrint() {
	return (PRINTDEBUG && _lastPC >= PRINTPC && _executionsRan >= PRINTEXE);
}