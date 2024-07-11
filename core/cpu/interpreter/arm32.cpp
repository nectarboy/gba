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
	}
}

// -- Data Processing Instructions --
// CPSR functions
void Arm32SetCPSR_ALU_LOG(struct Arm7* cpu, bool s, uint d, u64 res) { // (AND, EOR, TST, TEQ, ORR, MOV, BIC, MVN) 
	if (d == 15 || !s)
		return;

	cpu->cpsr.flagC = ((res >> 31) & 0b10) >> 1;
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
// Logical
void Arm32AND(struct Arm7* cpu, CC cc, bool s, uint d, uint n, u32 op2) {
	u64 res = cpu->writeReg(d, cpu->readReg(n) & op2);
	Arm32SetCPSR_ALU_LOG(cpu, s, d, res);
}
void Arm32EOR(struct Arm7* cpu, CC cc, bool s, uint d, uint n, u32 op2) {
	u64 res = cpu->writeReg(d, cpu->readReg(n) ^ op2);
	Arm32SetCPSR_ALU_LOG(cpu, s, d, res);
}
// Arithmetic
void Arm32SUB(struct Arm7* cpu, CC cc, bool s, uint d, uint n, u32 op2) {
	u32 a = cpu->readReg(n);
	u64 res = cpu->writeReg(d, a - op2);
	Arm32SetCPSR_ALU_ARI(cpu, s, d, a, op2, res);
}
void Arm32RSB(struct Arm7* cpu, CC cc, bool s, uint d, uint n, u32 op2) {
	u32 a = cpu->readReg(n);
	u64 res = cpu->writeReg(d, op2 - a);
	Arm32SetCPSR_ALU_ARI(cpu, s, d, a, op2, res);
}
void Arm32ADD(struct Arm7* cpu, CC cc, bool s, uint d, uint n, u32 op2) {
	u32 a = cpu->readReg(n);
	u64 res = cpu->writeReg(d, a + op2);
	Arm32SetCPSR_ALU_ARI(cpu, s, d, a, op2, res);
}
void Arm32ADC(struct Arm7 *cpu, CC cc, bool s, uint d, uint n, u32 op2) {
	u32 a = cpu->readReg(n) + cpu->cpsr.flagC;
	u64 res = cpu->writeReg(d, a + op2);
	Arm32SetCPSR_ALU_ARI(cpu, s, d, a, op2, res);
}
void Arm32SBC(struct Arm7* cpu, CC cc, bool s, uint d, uint n, u32 op2) {
	u32 a = cpu->readReg(n) + cpu->cpsr.flagC - 1;
	u64 res = cpu->writeReg(d, a - op2);
	Arm32SetCPSR_ALU_ARI(cpu, s, d, a, op2, res);
}
void Arm32RSC(struct Arm7* cpu, CC cc, bool s, uint d, uint n, u32 op2) {
	u32 a = cpu->readReg(n) - cpu->cpsr.flagC + 1;
	u64 res = cpu->writeReg(d, op2 - a);
	Arm32SetCPSR_ALU_ARI(cpu, s, d, a, op2, res);
}
void Arm32CMP(struct Arm7* cpu, CC cc, bool s, uint d, uint n, u32 op2) {
	u32 a = cpu->readReg(n);
	u64 res = a - op2;
	Arm32SetCPSR_ALU_ARI(cpu, s, d, a, op2, res);
}
void Arm32CMN(struct Arm7* cpu, CC cc, bool s, uint d, uint n, u32 op2) {
	u32 a = cpu->readReg(n);
	u64 res = a + op2;
	Arm32SetCPSR_ALU_ARI(cpu, s, d, a, op2, res);
}

// -- Branch Instructions -- //
void Arm32B(struct Arm7* cpu, CC cc, bool s, uint addr) {
	cpu->writeReg(15, addr);
}
void Arm32BIC(struct Arm7* cpu, CC cc, bool s, uint d, uint n, uint op2) {
	cpu->writeReg(d, cpu->readReg(n) & (~op2));
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