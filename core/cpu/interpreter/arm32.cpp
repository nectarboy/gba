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
	AL
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
	default: std::cout << "[!] UNDEFINED CONDITION CODE: " << cc << "\n";
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
void Arm32SetCPSR_ALU_LOG(struct Arm7* cpu, bool s, uint d, u64 res) { // (AND, EOR, TST, TEQ, ORR, MOV, BIC, MVN) 
	if (d == 15 || !s)
		return;

	cpu->cpsr.flagN = (res >> 31) & 1;
	cpu->cpsr.flagZ = (res & 0xffff'ffff) == 0;
}
void Arm32SetCPSR_ALU_ARI(struct Arm7* cpu, bool s, uint d, u32 a, u32 b, u64 res) { // (SUB, RSB, ADD, ADC, SBC, RSC, CMP, CMN) 
	if (d == 15 || !s)
		return;

	cpu->cpsr.flagC = ((res >> 31) & 0b10) >> 1;
	cpu->cpsr.flagN = (res >> 31) & 1;
	cpu->cpsr.flagZ = (res & 0xffff'ffff) == 0;
	cpu->cpsr.flagV = (cpu->cpsr.flagN) ^ ((a >> 31) & 1) ^ ((b >> 31) & 1);
}
// Bit Shifter
u32 ARM32_ALU_GetShiftedOperand(struct Arm7* cpu, bool i, u32 op2, bool affectFlagC) { // TODO: this last condition is quick and dirty and can be optimized later
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
	}
}
// Logical
void Arm32MOV(struct Arm7* cpu, CC cc, bool i, bool s, uint d, u32 op2) {
	op2 = ARM32_ALU_GetShiftedOperand(cpu, i, op2, true);
	cpu->writeReg(d, op2);
	Arm32SetCPSR_ALU_LOG(cpu, s, d, op2);
}
void Arm32MVN(struct Arm7* cpu, CC cc, bool i, bool s, uint d, u32 op2) {
	op2 = ARM32_ALU_GetShiftedOperand(cpu, i, op2, true);
	cpu->writeReg(d, ~op2);
	Arm32SetCPSR_ALU_LOG(cpu, s, d, ~op2);
}
void Arm32ORR(struct Arm7* cpu, CC cc, bool i, bool s, uint d, uint n, u32 op2) {
	op2 = ARM32_ALU_GetShiftedOperand(cpu, i, op2, true);
	u64 res = cpu->writeReg(d, cpu->readReg(n) | op2);
	Arm32SetCPSR_ALU_LOG(cpu, s, d, res);
}
void Arm32EOR(struct Arm7* cpu, CC cc, bool i, bool s, uint d, uint n, u32 op2) {
	op2 = ARM32_ALU_GetShiftedOperand(cpu, i, op2, true);
	u64 res = cpu->writeReg(d, cpu->readReg(n) ^ op2);
	Arm32SetCPSR_ALU_LOG(cpu, s, d, res);
}
void Arm32AND(struct Arm7* cpu, CC cc, bool i, bool s, uint d, uint n, u32 op2) {
	op2 = ARM32_ALU_GetShiftedOperand(cpu, i, op2, true);
	u64 res = cpu->writeReg(d, cpu->readReg(n) & op2);
	Arm32SetCPSR_ALU_LOG(cpu, s, d, res);
}
void Arm32BIC(struct Arm7* cpu, CC cc, bool i, bool s, uint d, uint n, uint op2) {
	op2 = ARM32_ALU_GetShiftedOperand(cpu, i, op2, true);
	u64 res = cpu->writeReg(d, cpu->readReg(n) & (~op2));
	Arm32SetCPSR_ALU_LOG(cpu, s, d, res);
}
void Arm32TST(struct Arm7* cpu, CC cc, bool i, bool s, uint n, uint op2) {
	op2 = ARM32_ALU_GetShiftedOperand(cpu, i, op2, true);
	u64 res = cpu->readReg(n) & op2;
	Arm32SetCPSR_ALU_LOG(cpu, s, 0, res);
}
void Arm32TEQ(struct Arm7* cpu, CC cc, bool i, bool s, uint n, uint op2) {
	op2 = ARM32_ALU_GetShiftedOperand(cpu, i, op2, true);
	u64 res = cpu->readReg(n) ^ op2;
	Arm32SetCPSR_ALU_LOG(cpu, s, 0, res);
}
// Arithmetic
void Arm32SUB(struct Arm7* cpu, CC cc, bool i, bool s, uint d, uint n, u32 op2) {
	op2 = ARM32_ALU_GetShiftedOperand(cpu, i, op2, true);
	u32 a = cpu->readReg(n);
	u64 res = cpu->writeReg(d, a - op2);
	Arm32SetCPSR_ALU_ARI(cpu, s, d, a, op2, res);
}
void Arm32RSB(struct Arm7* cpu, CC cc, bool i, bool s, uint d, uint n, u32 op2) {
	op2 = ARM32_ALU_GetShiftedOperand(cpu, i, op2, true);
	u32 a = cpu->readReg(n);
	u64 res = cpu->writeReg(d, op2 - a);
	Arm32SetCPSR_ALU_ARI(cpu, s, d, a, op2, res);
}
void Arm32ADD(struct Arm7* cpu, CC cc, bool i, bool s, uint d, uint n, u32 op2) {
	op2 = ARM32_ALU_GetShiftedOperand(cpu, i, op2, true);
	u32 a = cpu->readReg(n);
	u64 res = cpu->writeReg(d, a + op2);
	Arm32SetCPSR_ALU_ARI(cpu, s, d, a, op2, res);
}
void Arm32ADC(struct Arm7 *cpu, CC cc, bool i, bool s, uint d, uint n, u32 op2) {
	op2 = ARM32_ALU_GetShiftedOperand(cpu, i, op2, true);
	u32 a = cpu->readReg(n) + cpu->cpsr.flagC;
	u64 res = cpu->writeReg(d, a + op2);
	Arm32SetCPSR_ALU_ARI(cpu, s, d, a, op2, res);
}
void Arm32SBC(struct Arm7* cpu, CC cc, bool i, bool s, uint d, uint n, u32 op2) {
	op2 = ARM32_ALU_GetShiftedOperand(cpu, i, op2, true);
	u32 a = cpu->readReg(n) + cpu->cpsr.flagC - 1;
	u64 res = cpu->writeReg(d, a - op2);
	Arm32SetCPSR_ALU_ARI(cpu, s, d, a, op2, res);
}
void Arm32RSC(struct Arm7* cpu, CC cc, bool i, bool s, uint d, uint n, u32 op2) {
	op2 = ARM32_ALU_GetShiftedOperand(cpu, i, op2, true);
	u32 a = cpu->readReg(n) - cpu->cpsr.flagC + 1;
	u64 res = cpu->writeReg(d, op2 - a);
	Arm32SetCPSR_ALU_ARI(cpu, s, d, a, op2, res);
}
void Arm32CMP(struct Arm7* cpu, CC cc, bool i, bool s, uint n, u32 op2) {
	op2 = ARM32_ALU_GetShiftedOperand(cpu, i, op2, true);
	u32 a = cpu->readReg(n);
	u64 res = a - op2;
	Arm32SetCPSR_ALU_ARI(cpu, s, 0, a, op2, res);
}
void Arm32CMN(struct Arm7* cpu, CC cc, bool i, bool s, uint n, u32 op2) {
	op2 = ARM32_ALU_GetShiftedOperand(cpu, i, op2, true);
	u32 a = cpu->readReg(n);
	u64 res = a + op2;
	Arm32SetCPSR_ALU_ARI(cpu, s, 0, a, op2, res);
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
void Arm32MUL(struct Arm7* cpu, CC cc, bool a, bool s, uint rd, uint rn, uint rs, uint rm) {
	u32 res = (u64)(cpu->readReg(rm)) * (u64)(cpu->readReg(rs)) + (u64)(cpu->readReg(rn) * a);
	cpu->writeReg(rd, res);
	Arm32SetCPSR_MUL32(cpu, s, res);
}
// -- Long (64-bit) Multiplication -- (CC, A, S, RdHi, RdLo, Rs, Rm)
void Arm32UMULL(struct Arm7* cpu, CC cc, bool a, bool s, uint rdHi, uint rdLo, uint rs, uint rm) {
	u64 res = (u64)(cpu->readReg(rm)) * (u64)(cpu->readReg(rs)) + a * ((u64(cpu->readReg(rdHi)) << 32) | cpu->readReg(rdLo));
	cpu->writeReg(rdHi, res >> 32);
	cpu->writeReg(rdLo, res);
	Arm32SetCPSR_MUL64(cpu, s, res);
}
void Arm32SMULL(struct Arm7* cpu, CC cc, bool a, bool s, uint rdHi, uint rdLo, uint rs, uint rm) {
	u64 res = (s64)((s32)(cpu->readReg(rm))) * (s64)((s32)(cpu->readReg(rs))) + a * ((u64(cpu->readReg(rdHi)) << 32) | cpu->readReg(rdLo));
	cpu->writeReg(rdHi, res >> 32);
	cpu->writeReg(rdLo, res);
	Arm32SetCPSR_MUL64(cpu, s, res);
}

// -- Single Data Transfer Instructions -- //
// Below is possibly bugged
u32 ARM32_LDR_GetShiftedOperand(struct Arm7* cpu, bool i, u32 off) { // TODO: this last condition is quick and dirty and can be optimized later
	// 12-bit Immediate Value
	if (!i) {
		return off;
	}
	// Register Value
	else {
		uint reg = off & 0xf; // Index of Rs
		u32 val = cpu->readReg(reg);
		uint shift;

		assert((op2 >> 5) & 1 == 0); // Shift amt can only be immediate (bit 4 must be 0);
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
	}
}
void ARM32LDR(struct Arm7* cpu, CC cc, bool i, bool p, bool u, bool b, bool w, bool l, uint rn, uint rd, u32 off) {
	off = ARM32_LDR_GetShiftedOperand(cpu, i, off);
	// Apparently GBA only use little endianness. TODO: confirm this
}

// Decoding
void Arm32Decode(struct Arm7* cpu, u32 opc) {
	CC cc = (CC)((opc >> 31) & 0b11);
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