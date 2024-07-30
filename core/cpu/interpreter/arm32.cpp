#include <iostream>
#include "helpers.h"

// Condition Codes (i hope they work this time)
enum CC {
	Z_SET,
	Z_CLR,
	C_SET,
	C_CLR,
	N_SET,
	N_CLR,
	V_SET,
	V_CLR,
	C_SET_AND_Z_CLR,
	C_CLR_OR_Z_SET,
	N_EQ_V,
	N_NEQ_V,
	Z_CLR_AND_N_EQ_V,
	Z_SET_OR_N_NEQ_V,
	AL,
	UND
};
bool evalConditionCode(struct Arm7* cpu, CC cc) {
	switch (cc) {
	case Z_SET: return cpu->cpsr.flagZ;
	case Z_CLR: return !cpu->cpsr.flagZ;
	case C_SET: return cpu->cpsr.flagC;
	case C_CLR: return !cpu->cpsr.flagC;
	case N_SET: return cpu->cpsr.flagN;
	case N_CLR: return !cpu->cpsr.flagN;
	case V_SET: return cpu->cpsr.flagV;
	case V_CLR: return !cpu->cpsr.flagV;
	case C_SET_AND_Z_CLR: return cpu->cpsr.flagC & !cpu->cpsr.flagZ;
	case C_CLR_OR_Z_SET: return !cpu->cpsr.flagC | cpu->cpsr.flagZ;
	case N_EQ_V: return cpu->cpsr.flagN == cpu->cpsr.flagV;
	case N_NEQ_V: return cpu->cpsr.flagN != cpu->cpsr.flagV;
	case Z_CLR_AND_N_EQ_V: return !cpu->cpsr.flagZ & (cpu->cpsr.flagN == cpu->cpsr.flagV);
	case Z_SET_OR_N_NEQ_V: return cpu->cpsr.flagZ | (cpu->cpsr.flagN != cpu->cpsr.flagV);
	case AL: return true;
	case UND: return false;
	default: std::cout << "[!] UNDEFINED CONDITION CODE: " << cc << "\n"; assert(0);
	}
}

// -- Branch Instructions -- //
void Arm32BX(struct Arm7* cpu, CC cc, uint n) {
	cpu->writeReg(15, cpu->readReg(n));
	cpu->setThumbMode((bool)(cpu->readReg(n) & 1)); // OPT
}
void Arm32BL(struct Arm7* cpu, CC cc, bool l, u32 off) {
	// Shift left 2 bits, sign extend to 32 bits
	off <<= 8;
	off = bitSignedShiftRight(off, 32, 6);
	s32 soff = (s32)(off);

	u32 pc = cpu->readReg(15);
	if (l)
		cpu->writeReg(14, pc);
	cpu->writeReg(15, pc + soff);
}

// -- Data Processing Instructions -- (CC, L, S, Rn, Rd, Op2), 16 instructions in total
// CPSR functions
void Arm32_DataProcessing_Logical_SetCPSR(struct Arm7* cpu, bool s, uint d, u64 res) { // (AND, EOR, TST, TEQ, ORR, MOV, BIC, MVN) 
	if (d == 15 || !s) {
		return;
	}
	else if (d == 15) {
		cpu->copySPSRToCPSR(); // CONFIRM if this is correct; refer to 4.5.4
	}
	else {
		cpu->cpsr.flagN = (res >> 31) & 1;
		cpu->cpsr.flagZ = (res & 0xffff'ffff) == 0;
	}
}
void Arm32_DataProcessing_Arithmetic_SetCPSR(struct Arm7* cpu, bool s, uint d, u32 a, u32 b, u64 res) { // (SUB, RSB, ADD, ADC, SBC, RSC, CMP, CMN) 
	if (d == 15 || !s) {
		return;
	}
	else if (d == 15) {
		cpu->copySPSRToCPSR(); // CONFIRM if this is correct; refer to 4.5.4
	}
	else {
		cpu->cpsr.flagC = ((res >> 31) & 0b10) >> 1;
		cpu->cpsr.flagN = (res >> 31) & 1;
		cpu->cpsr.flagZ = (res & 0xffff'ffff) == 0;
		cpu->cpsr.flagV = (cpu->cpsr.flagN) ^ ((a >> 31) & 1) ^ ((b >> 31) & 1);
	}
}
// Bit Shifter
u32 Arm32_DataProcessing_GetShiftedOperand(struct Arm7* cpu, bool i, u32 op2, bool affectFlagC) { // TODO: this last condition is quick and dirty and can be optimized later
	// 8-bit Immediate Value
	if (i) {
		uint shift = (op2 >> 8);
		u32 val = op2 & 0xff; // 8 bit immediate zero extended to 32 bits
		// TODO: how is flagC affected ??
		return bitRotateRight(val, 32, shift << 2); // ROR by twice the shift ammount
	}
	// Register Value
	else {
		uint reg = op2 & 0xf; // Index of Rs
		u32 val = cpu->readReg(reg);
		uint shift;

		if (op2 & 0b10000) {
			assert((op2 >> 7) & 1 == 0); // "The zero in bit 7 of an instruction with a register controlled shift is compulsory; a one in this bit will cause the instruction to be a multiply or undefined instruction."

			shift = val & 0xff; // Shift ammount determined by last byte of Rs
			if (shift == 0)
				return val;
		}	
		else {
			shift = (op2 >> 7) & 0b11111; // Shift ammount is a 5 bit immediate
		}

		uint shifttype = (op2 >> 5) & 0b11;
		switch (shifttype) {
		case 0b00:
			if (affectFlagC && shift != 0)
				cpu->cpsr.flagC = 1 & bitShiftLeft(val, 32, 32 - shift); // Should work for >= 32
			return bitShiftLeft(val, 32, shift); // Logical left
		case 0b01:
			if (shift == 0)
				shift = 32;
			if (affectFlagC)
				cpu->cpsr.flagC = 1 & bitShiftRight(val, 32, shift - 1); // Should work for >= 32
			return bitShiftRight(val, 32, shift); // Logical right
		case 0b10:
			if (shift == 0 || shift > 32)
				shift = 32;
			if (affectFlagC)
				cpu->cpsr.flagC = 1 & bitShiftRight(val, 32, shift - 1);
			return bitSignedShiftRight(val, 32, shift); // Arithmetic (signed) right.
		case 0b11:
			shift &= 0b11111;
			// RRX
			if (shift == 0) {
				int oldFlagC = cpu->cpsr.flagC;
				if (affectFlagC)
					cpu->cpsr.flagC = val & 1;
				return (val >> 1) | (oldFlagC << 31);
			}
			// Normal ROR
			else {
				if (affectFlagC)
					cpu->cpsr.flagC = 1 & (val >> (shift - 1));
				return bitRotateRight(val, 32, shift);// Rotate right
			}
		}

		assert(0);
	}
}
void Arm32_DataProcessing(Arm7* cpu, u32 instruction) {
	bool i = (instruction >> 25) & 1;
	uint opcode = (instruction >> 21) & 0xf;
	bool s = (instruction >> 20) & 1;
	uint rn = (instruction >> 16) & 0xf;
	uint rd = (instruction >> 12) & 0xf;
	u32 op2 = (instruction >> 0) & 0xfff;

	op2 = Arm32_DataProcessing_GetShiftedOperand(cpu, i, op2, true);

	switch (opcode) {
	case 0: { // AND
		u64 res = cpu->writeReg(rd, cpu->readReg(rn) & op2);
		Arm32_DataProcessing_Logical_SetCPSR(cpu, s, rd, res);
		break;}
	case 1: { // EOR
		u64 res = cpu->writeReg(rd, cpu->readReg(rn) ^ op2);
		Arm32_DataProcessing_Logical_SetCPSR(cpu, s, rd, res);
		break;}
	case 2: { // SUB
		u32 a = cpu->readReg(rn);
		u64 res = cpu->writeReg(rd, a - op2);
		Arm32_DataProcessing_Arithmetic_SetCPSR(cpu, s, rd, a, op2, res);
		break;}
	case 3: { // RSB
		u32 a = cpu->readReg(rn);
		u64 res = cpu->writeReg(rd, op2 - a);
		Arm32_DataProcessing_Arithmetic_SetCPSR(cpu, s, rd, a, op2, res);
		break;}
	case 4: { // ADD
		u32 a = cpu->readReg(rn);
		u64 res = cpu->writeReg(rd, a + op2);
		Arm32_DataProcessing_Arithmetic_SetCPSR(cpu, s, rd, a, op2, res); \
			break;}
	case 5: { // ADC
		u32 a = cpu->readReg(rn) + cpu->cpsr.flagC;
		u64 res = cpu->writeReg(rd, a + op2);
		Arm32_DataProcessing_Arithmetic_SetCPSR(cpu, s, rd, a, op2, res);
		break;}
	case 6: { // SBC
		u32 a = cpu->readReg(rn) + cpu->cpsr.flagC - 1;
		u64 res = cpu->writeReg(rd, a - op2);
		Arm32_DataProcessing_Arithmetic_SetCPSR(cpu, s, rd, a, op2, res);
		break;}
	case 7: { // RSC
		u32 a = cpu->readReg(rn) - cpu->cpsr.flagC + 1;
		u64 res = cpu->writeReg(rd, op2 - a);
		Arm32_DataProcessing_Arithmetic_SetCPSR(cpu, s, rd, a, op2, res);
		break;}
	case 8: { // TST
		u64 res = cpu->readReg(rn) & op2;
		Arm32_DataProcessing_Logical_SetCPSR(cpu, s, 0, res);
		break;}
	case 9: { // TEQ
		u64 res = cpu->readReg(rn) ^ op2;
		Arm32_DataProcessing_Logical_SetCPSR(cpu, s, 0, res);
		break;}
	case 10: { // CMP
		u32 a = cpu->readReg(rn);
		u64 res = a - op2;
		Arm32_DataProcessing_Arithmetic_SetCPSR(cpu, s, 0, a, op2, res);
		break;}
	case 11: { // CMN
		u32 a = cpu->readReg(rn);
		u64 res = a + op2;
		Arm32_DataProcessing_Arithmetic_SetCPSR(cpu, s, 0, a, op2, res);
		break;}
	case 12: { // ORR
		u64 res = cpu->writeReg(rd, cpu->readReg(rn) | op2);
		Arm32_DataProcessing_Logical_SetCPSR(cpu, s, rd, res);
		break;}
	case 13: { // MOV
		cpu->writeReg(rd, op2);
		Arm32_DataProcessing_Logical_SetCPSR(cpu, s, rd, op2);
		break;}
	case 14: { // BIC
		u64 res = cpu->writeReg(rd, cpu->readReg(rn) & (~op2));
		Arm32_DataProcessing_Logical_SetCPSR(cpu, s, rd, res);
		break;}
	case 15: { // MVN
		cpu->writeReg(rd, ~op2);
		Arm32_DataProcessing_Logical_SetCPSR(cpu, s, rd, ~op2);
		break;}
	}
}

// -- PSR Transfer Instructions -- //

// -- Multiplication Instructions -- //
inline void Arm32SetCPSR_MUL32(struct Arm7* cpu, bool s, u32 res) { // TODO: can this be a template ? idk what templates are but this seems like it could be one
	if (!s)
		return;

	cpu->cpsr.flagN = res >> 31;
	cpu->cpsr.flagZ = res == 0;
	//cpu->cpsr.flagC = false;
}
inline void Arm32SetCPSR_MUL64(struct Arm7* cpu, bool s, u64 res) {
	if (!s)
		return;

	cpu->cpsr.flagN = res >> 63;
	cpu->cpsr.flagZ = res == 0;
	//cpu->cpsr.flagC = false;
	//cpu->cpsr.flagV = false;
}
// -- 32-bit Multiplication -- (CC, A, S, Rd, Rn, Rs, Rm)
void Arm32_Multiply(struct Arm7* cpu, CC cc, bool a, bool s, uint rd, uint rn, uint rs, uint rm) {
	u32 res = (u64)(cpu->readReg(rm)) * (u64)(cpu->readReg(rs)) + (u64)(cpu->readReg(rn) * a);
	cpu->writeReg(rd, res);
	Arm32SetCPSR_MUL32(cpu, s, res);
}
// -- Long (64-bit) Multiplication -- (CC, A, S, RdHi, RdLo, Rs, Rm)
// TODO: convert the signed and unsigned into one function, its ok well make it a template later
void Arm32_MultiplyLong_Unsigned(struct Arm7* cpu, CC cc, bool a, bool s, uint rdHi, uint rdLo, uint rs, uint rm) {
	u64 res = (u64)(cpu->readReg(rm)) * (u64)(cpu->readReg(rs)) + a * ((u64(cpu->readReg(rdHi)) << 32) | cpu->readReg(rdLo));
	cpu->writeReg(rdHi, res >> 32);
	cpu->writeReg(rdLo, res);
	Arm32SetCPSR_MUL64(cpu, s, res);
}
void Arm32_MultiplyLong_Signed(struct Arm7* cpu, CC cc, bool a, bool s, uint rdHi, uint rdLo, uint rs, uint rm) {
	u64 res = (s64)((s32)(cpu->readReg(rm))) * (s64)((s32)(cpu->readReg(rs))) + a * ((u64(cpu->readReg(rdHi)) << 32) | cpu->readReg(rdLo));
	cpu->writeReg(rdHi, res >> 32);
	cpu->writeReg(rdLo, res);
	Arm32SetCPSR_MUL64(cpu, s, res);
}

// -- Single Data Transfer Instructions -- //
// Below is possibly bugged
u32 Arm32_SingleDataTransfer_GetShiftedOffset(struct Arm7* cpu, bool i, u32 off) { // TODO: this last condition is quick and dirty and can be optimized later
	// 12-bit Immediate Value
	if (!i) {
		return off;
	}
	// Register Value
	else {
		uint reg = off & 0xf; // Index of Rs
		u32 val = cpu->readReg(reg);
		uint shift;

		assert((off >> 5) & 1 == 0); // Shift amt can only be immediate (bit 4 must be 0);
		shift = (off >> 7) & 0b11111;

		uint shifttype = (off >> 5) & 0b11;
		switch (shifttype) {
		case 0b00:
			return bitShiftLeft(val, 32, shift); // Logical left
		case 0b01:
			if (shift == 0)
				shift = 32;
			return bitShiftRight(val, 32, shift); // Logical right
		case 0b10:
			if (shift == 0 || shift > 32)
				shift = 32;
			return bitSignedShiftRight(val, 32, shift); // Arithmetic (signed) right.
		case 0b11:
			shift &= 0b11111;
			if (shift == 0)
				return (val >> 1) | (cpu->cpsr.flagC << 31); // RRX
			else
				return bitRotateRight(val, 32, shift); // Normal ROR
		}
		assert(0);
	}
}
void Arm32_SingleDataTransfer(struct Arm7* cpu, u32 instruction) {
	bool i = (instruction >> 25) & 1;
	bool p = (instruction >> 24) & 1;
	bool u = (instruction >> 23) & 1;
	bool b = (instruction >> 22) & 1;
	bool w = (instruction >> 21) & 1;
	bool l = (instruction >> 20) & 1;
	uint rn = (instruction >> 16) & 0xf;
	uint rd = (instruction >> 12) & 0xf;
	uint off = (instruction >> 0) & 0xfff;

	u32 base = cpu->readReg(rn);
	off = Arm32_SingleDataTransfer_GetShiftedOffset(cpu, i, off);
	if (!u)
		off = ~off + 1; // Negative
	u32 addr = (base + off * p) & 0xffff'fffc; // Align to word boundary

	// TODO: r15 reads should return +12 !!!
	// Load
	if (l) {
		if (b)
			cpu->writeRegBottomByte(rd, cpu->read8(addr));
		else
			cpu->writeReg(rd, cpu->read32(addr));
	}
	// Store
	else {
		if (b)
			cpu->write8(addr, u8(cpu->readReg(rd)));
		else
			cpu->write32(addr, cpu->readReg(rd));
	}

	if (w)
		cpu->writeReg(rn, base + off);
}
void Arm32_HalfwordSignedDataTransfer(struct Arm7* cpu, u32 instruction) {
	// CONFIRM: Bit 22 can be both 0 or 1 ?
	bool p = (instruction >> 24) & 1;
	bool u = (instruction >> 23) & 1;
	bool w = (instruction >> 21) & 1;
	bool l = (instruction >> 20) & 1;
	uint rn = (instruction >> 16) & 0xf;
	uint rd = (instruction >> 12) & 0xf;
	bool s = (instruction >> 6) & 1;
	bool h = (instruction >> 5) & 1;
	//uint type = (instruction >> 5) & 0b11;
	uint rm = (instruction >> 0) & 0xf;

	u32 base = cpu->readReg(rn);
	u32 off = cpu->readReg(rm);
	if (!u)
		off = ~off + 1; // Negative

	//switch (type) {
	//case 0b00: { // Swap
	//	break;
	//}
	//case 0b01: { // Unsigned Halfword
	//	break;
	//}
	//case 0b10: { // Signed byte transfer
	//	break;
	//}
	//case 0b11: { // Signed halfword transfer
	//	break;
	//}
	//}

	// Load
	if (l) {
		if (h)
			cpu->writeRegBottomHalfword(rd, cpu->read16OptionalSign(base + off * p, s));
		else
			cpu->writeRegBottomByte(rd, cpu->read8OptionalSign(base + off * p, s));
	}
	// Store
	else {
		if (h)
			cpu->write8(base + off * p, u8(cpu->readReg(rd)));
		else
			cpu->write32(base + off * p, cpu->readReg(rd));
	}

	if (w)
		cpu->writeReg(rn, base + off);
}

// Decoding
void Arm32Decode(struct Arm7* cpu, u32 instruction) {
	CC cc = CC((instruction >> 28) & 0xf);
}

// My charlie brown ascii art. if you even care
//   _____
//  / 6c  \
// |(·c·)  |
//<|  _    |>
//  \_____/
//   /|_|\
//  |     |
//  |   |_|
//  |\/\| |
//  |___|_/
//  |_|__|
//   ||_|
//  [_[__|