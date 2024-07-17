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

// -- Data Processing Instructions -- (CC, Opc, S, Rn, Rd, Op2), 16 instructions in total
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
u32 ARM32_ALU_GetShiftedOperand(struct Arm7* cpu, bool l, u32 op2) {
	// 8-bit Immediate Value
	if (l) {
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
			if (shift != 0)
				cpu->cpsr.flagC = 1 & bitShiftLeft(val, 32, 32 - shift); // Should work for >= 32
			return bitShiftLeft(val, 32, shift); // Logical left
		case 0b01:
			if (shift == 0)
				shift = 32;
			cpu->cpsr.flagC = 1 & bitShiftRight(val, 32, shift - 1); // Should work for >= 32
			return bitShiftRight(val, 32, shift); // Logical right
		case 0b10:
			if (shift == 0 || shift > 32)
				shift = 32;
			cpu->cpsr.flagC = 1 & bitShiftRight(val, 32, shift - 1);
			return bitSignedShiftRight(val, 32, shift); // Arithmetic (signed) right.
		case 0b11:
			shift &= 0b11111;
			// RRX
			if (shift == 0) {
				int oldFlagC = cpu->cpsr.flagC;
				cpu->cpsr.flagC = val & 1;
				return (val >> 1) | (oldFlagC << 31);
			}
			// Normal ROR
			else {
				cpu->cpsr.flagC = 1 & (val >> (shift - 1));
				return bitRotateRight(val, 32, shift);// Rotate right
			}
		}
	}
}
// Logical
void Arm32MOV(struct Arm7* cpu, CC cc, bool l, bool s, uint d, u32 op2) {
	op2 = ARM32_ALU_GetShiftedOperand(cpu, l, op2);
	cpu->writeReg(d, op2);
	Arm32SetCPSR_ALU_LOG(cpu, s, d, op2);
}
void Arm32MVN(struct Arm7* cpu, CC cc, bool l, bool s, uint d, u32 op2) {
	op2 = ARM32_ALU_GetShiftedOperand(cpu, l, op2);
	cpu->writeReg(d, ~op2);
	Arm32SetCPSR_ALU_LOG(cpu, s, d, ~op2);
}
void Arm32ORR(struct Arm7* cpu, CC cc, bool l, bool s, uint d, uint n, u32 op2) {
	op2 = ARM32_ALU_GetShiftedOperand(cpu, l, op2);
	u64 res = cpu->writeReg(d, cpu->readReg(n) | op2);
	Arm32SetCPSR_ALU_LOG(cpu, s, d, res);
}
void Arm32EOR(struct Arm7* cpu, CC cc, bool l, bool s, uint d, uint n, u32 op2) {
	op2 = ARM32_ALU_GetShiftedOperand(cpu, l, op2);
	u64 res = cpu->writeReg(d, cpu->readReg(n) ^ op2);
	Arm32SetCPSR_ALU_LOG(cpu, s, d, res);
}
void Arm32AND(struct Arm7* cpu, CC cc, bool l, bool s, uint d, uint n, u32 op2) {
	op2 = ARM32_ALU_GetShiftedOperand(cpu, l, op2);
	u64 res = cpu->writeReg(d, cpu->readReg(n) & op2);
	Arm32SetCPSR_ALU_LOG(cpu, s, d, res);
}
void Arm32BIC(struct Arm7* cpu, CC cc, bool l, bool s, uint d, uint n, uint op2) {
	op2 = ARM32_ALU_GetShiftedOperand(cpu, l, op2);
	u64 res = cpu->writeReg(d, cpu->readReg(n) & (~op2));
	Arm32SetCPSR_ALU_LOG(cpu, s, d, res);
}
void Arm32TST(struct Arm7* cpu, CC cc, bool l, bool s, uint n, uint op2) {
	op2 = ARM32_ALU_GetShiftedOperand(cpu, l, op2);
	u64 res = cpu->readReg(n) & op2;
	Arm32SetCPSR_ALU_LOG(cpu, s, 0, res);
}
void Arm32TEQ(struct Arm7* cpu, CC cc, bool l, bool s, uint n, uint op2) {
	op2 = ARM32_ALU_GetShiftedOperand(cpu, l, op2);
	u64 res = cpu->readReg(n) ^ op2;
	Arm32SetCPSR_ALU_LOG(cpu, s, 0, res);
}
// Arithmetic
void Arm32SUB(struct Arm7* cpu, CC cc, bool l, bool s, uint d, uint n, u32 op2) {
	op2 = ARM32_ALU_GetShiftedOperand(cpu, l, op2);
	u32 a = cpu->readReg(n);
	u64 res = cpu->writeReg(d, a - op2);
	Arm32SetCPSR_ALU_ARI(cpu, s, d, a, op2, res);
}
void Arm32RSB(struct Arm7* cpu, CC cc, bool l, bool s, uint d, uint n, u32 op2) {
	op2 = ARM32_ALU_GetShiftedOperand(cpu, l, op2);
	u32 a = cpu->readReg(n);
	u64 res = cpu->writeReg(d, op2 - a);
	Arm32SetCPSR_ALU_ARI(cpu, s, d, a, op2, res);
}
void Arm32ADD(struct Arm7* cpu, CC cc, bool l, bool s, uint d, uint n, u32 op2) {
	op2 = ARM32_ALU_GetShiftedOperand(cpu, l, op2);
	u32 a = cpu->readReg(n);
	u64 res = cpu->writeReg(d, a + op2);
	Arm32SetCPSR_ALU_ARI(cpu, s, d, a, op2, res);
}
void Arm32ADC(struct Arm7 *cpu, CC cc, bool l, bool s, uint d, uint n, u32 op2) {
	op2 = ARM32_ALU_GetShiftedOperand(cpu, l, op2);
	u32 a = cpu->readReg(n) + cpu->cpsr.flagC;
	u64 res = cpu->writeReg(d, a + op2);
	Arm32SetCPSR_ALU_ARI(cpu, s, d, a, op2, res);
}
void Arm32SBC(struct Arm7* cpu, CC cc, bool l, bool s, uint d, uint n, u32 op2) {
	op2 = ARM32_ALU_GetShiftedOperand(cpu, l, op2);
	u32 a = cpu->readReg(n) + cpu->cpsr.flagC - 1;
	u64 res = cpu->writeReg(d, a - op2);
	Arm32SetCPSR_ALU_ARI(cpu, s, d, a, op2, res);
}
void Arm32RSC(struct Arm7* cpu, CC cc, bool l, bool s, uint d, uint n, u32 op2) {
	op2 = ARM32_ALU_GetShiftedOperand(cpu, l, op2);
	u32 a = cpu->readReg(n) - cpu->cpsr.flagC + 1;
	u64 res = cpu->writeReg(d, op2 - a);
	Arm32SetCPSR_ALU_ARI(cpu, s, d, a, op2, res);
}
void Arm32CMP(struct Arm7* cpu, CC cc, bool l, bool s, uint n, u32 op2) {
	op2 = ARM32_ALU_GetShiftedOperand(cpu, l, op2);
	u32 a = cpu->readReg(n);
	u64 res = a - op2;
	Arm32SetCPSR_ALU_ARI(cpu, s, 0, a, op2, res);
}
void Arm32CMN(struct Arm7* cpu, CC cc, bool l, bool s, uint n, u32 op2) {
	op2 = ARM32_ALU_GetShiftedOperand(cpu, l, op2);
	u32 a = cpu->readReg(n);
	u64 res = a + op2;
	Arm32SetCPSR_ALU_ARI(cpu, s, 0, a, op2, res);
}

// -- PSR Transfer Instructions -- //
// -- Multiplication Instructions -- //
// -- Long Multiplication Instructions -- //


// -- Branch Instructions -- //
void Arm32B(struct Arm7* cpu, CC cc, bool s, uint addr) {
	cpu->writeReg(15, addr);
}
void Arm32BL(struct Arm7* cpu, CC cc, bool s, uint addr) {
	cpu->writeReg(14, cpu->readReg(15));
	cpu->writeReg(15, addr);
}
void Arm32BX(struct Arm7* cpu, CC cc, bool s, uint n) {
	cpu->writeReg(15, cpu->readReg(n));
	cpu->setThumbMode((bool)(cpu->readReg(n) & 1)); // OPT
}
void Arm32CDP(struct Arm7* cpu) {}

// Decoding
void Arm32Decode(struct Arm7* cpu, u32 opc) {
	CC cc = (CC)((opc >> 31) & 0b11);
}