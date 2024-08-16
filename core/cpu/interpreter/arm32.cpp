#include <iostream>
#include "helpers.h"
#include "constants.cpp"

// -- Branch Instructions -- //
void Arm32_BranchAndExchange(Arm7* cpu, u32 instruction) {
	if (!evalConditionCode(cpu, CC((instruction >> 28) & 0xf)))
		return;
	uint rn = instruction & 0xf;
		
	if (cpu->readReg(rn) & 1)
		print("BX TO THUMB");
	cpu->setThumbMode((bool)(cpu->readReg(rn) & 1)); // TODO: implement thumb
	cpu->writeReg(15, cpu->readReg(rn));
}
void Arm32_BranchAndLink(Arm7* cpu, u32 instruction) {
	if (!evalConditionCode(cpu, CC((instruction >> 28) & 0xf)))
		return;
	bool l = (instruction >> 24) & 1;
	u32 off = instruction & 0xff'ffff;

	// Shift left 2 bits, sign extend to 32 bits
	// Method 1 (not confident in this): 
	//off <<= 8;
	//off = bitSignedShiftRight(off, 32, 6);
	//s32 soff = (s32)(off);
	// Method 2:
	off |= 0xff00'f000 * (off >> 23);
	off <<= 2; // Now 26 bits
	s32 soff = s32(off);

	u32 pc = cpu->reg[15];
	if (l)
		cpu->writeReg(14, pc);
	cpu->writeReg(15, pc + 4 + soff);
}

// -- Data Processing Instructions -- (CC, L, S, Rn, Rd, Op2), 16 instructions in total
// TODO: these templates are kind of a fucking mess, maybe split this up for each instruction seperately sometime
// CPSR functions
template <bool unused_rd>
void Arm32_DataProcessing_Logical_SetCPSR(Arm7* cpu, bool s, uint d, u64 res) { 
	if (!s) {
		return;
	}
	else if (d == 15) {
		//if constexpr (!unused_rd)
			cpu->copySPSRToCPSR(); // CONFIRM if this is correct; refer to 4.5.4
	}
	else {
		cpu->cpsr.flagN = (res >> 31) & 1;
		cpu->cpsr.flagZ = u32(res) == 0;
	}
}

template <bool subtraction, bool unused_rd>
void Arm32_DataProcessing_Arithmetic_SetCPSR(Arm7* cpu, bool s, uint d, u32 a, u32 b, u64 res) { 
	if (!s) {
		return;
	}
	else if (d == 15) {
		//if constexpr (!unused_rd)
			cpu->copySPSRToCPSR(); // CONFIRM if this is correct; refer to 4.5.4
	}
	else {
		//cpu->cpsr.flagC = res > 0xffff'ffff;//(((res >> 31) >> 1) & 1) ^ ((res >> 31) & 1); // TODO: research this
		cpu->cpsr.flagN = (res >> 31) & 1;
		cpu->cpsr.flagZ = (res & 0xffff'ffff) == 0;
		//cpu->cpsr.flagV = (res > 0xffff'ffff && res < ~u64(0xffff'ffffUL));//(cpu->cpsr.flagN) ^ (((a >> 31) & 1) ^ ((b >> 31) & 1)); // TODO: may be fucked

		if constexpr (!subtraction) {
			cpu->cpsr.flagC = u32(res) < a;
			cpu->cpsr.flagV = (~(a ^ b) & (a ^ u32(res))) >> 31;//(u32(res) ^ a ^ b) >> 31;
		}
		else {
			cpu->cpsr.flagC = a >= b;
			cpu->cpsr.flagV = ((a ^ b) & (a ^ u32(res))) >> 31;
		}
	}
}
// Bit Shifter
u32 Arm32_DataProcessing_GetShiftedOperand(Arm7* cpu, bool i, u32 op2, u32* rd15offset, bool affectFlagC) { // TODO: this last condition is quick and dirty and can be optimized later
	// 8-bit Immediate Value
	if (i) {
		*rd15offset = 4;
		uint shift = (op2 >> 8) & 0xf;
		u32 imm = op2 & 0xff; // 8 bit immediate zero extended to 32 bits
		if (affectFlagC && shift != 0)
			cpu->cpsr.flagC = 1 & (imm >> (0b11111 & u32(shift*2 - 1)));
		return bitRotateRight(imm, 32, shift*2); // ROR by twice the shift ammount
	}
	// Register Value
	else {
		uint rm = op2 & 0xf; // Index of Rs
		uint shift;

		if (op2 & 0b10000) {
			*rd15offset = 8;
			assert(((op2 >> 7) & 1) == 0); // "The zero in bit 7 of an instruction with a register controlled shift is compulsory; a one in this bit will cause the instruction to be a multiply or undefined instruction."
			uint rs = (op2 >> 8);
			shift = (cpu->reg[rs] + *rd15offset * (rs == 15)) & 0xff;
			if (shift == 0)
				return cpu->reg[rm] + *rd15offset * (rm == 15);
		}	
		else {
			*rd15offset = 4;
			shift = (op2 >> 7) & 0b11111; // Shift ammount is a 5 bit immediate
		}
		u64 val = cpu->reg[rm] + *rd15offset*(rm==15);

		uint shifttype = (op2 >> 5) & 0b11;
		switch (shifttype) {
		case 0b00: // LSL
			if (affectFlagC && shift != 0)
				cpu->cpsr.flagC = 1 & bitShiftRight(val, 32, 32 - shift); // Should work for >= 32
			return bitShiftLeft(val, 32, shift); // Logical left
		case 0b01: // LSR
			if (shift == 0)
				shift = 32;
			if (affectFlagC)
				cpu->cpsr.flagC = 1 & bitShiftRight(val, 32, shift - 1); // Should work for >= 32
			return bitShiftRight(val, 32, shift); // Logical right
		case 0b10: // ASR
			if (shift == 0 || shift > 32)
				shift = 32;
			if (affectFlagC)
				cpu->cpsr.flagC = 1 & bitShiftRight(val, 32, shift - 1);
			return bitSignedShiftRight(val, 32, shift); // Arithmetic (signed) right.
		case 0b11: // ROR
			//shift &= 0b11111;
			// RRX
			if (shift == 0) {
				int oldFlagC = cpu->cpsr.flagC;
				if (affectFlagC)
					cpu->cpsr.flagC = val & 1;
				return (val >> 1) | (oldFlagC << 31);
			}
			// Normal ROR
			else {
				shift &= 0b11111;
				if (affectFlagC)
					cpu->cpsr.flagC = 1 & (val >> (0b11111 & u32(shift - 1)));
				return bitRotateRight(val, 32, shift);// Rotate right
			}
		}

		assert(0);
	}
}
void Arm32_DataProcessing(Arm7* cpu, u32 instruction) {
	if (!evalConditionCode(cpu, CC((instruction >> 28) & 0xf)))
		return;
	bool i = (instruction >> 25) & 1;
	uint opcode = (instruction >> 21) & 0xf;
	bool s = (instruction >> 20) & 1;
	uint rn = (instruction >> 16) & 0xf;
	uint rd = (instruction >> 12) & 0xf;
	u32 op2 = (instruction >> 0) & 0xfff;

	u32 rd15offset = 0;

	//op2 = Arm32_DataProcessing_GetShiftedOperand(cpu, i, op2, true);
	//if (i)
	//	std::cout << "dingle: " << std::hex << op2 << std::dec << "\n";

	switch (opcode) {
	case 0: { // AND
		op2 = Arm32_DataProcessing_GetShiftedOperand(cpu, i, op2, &rd15offset, true);
		u64 res = cpu->writeReg(rd, (cpu->reg[rn] + rd15offset*(rn==15)) & op2);
		Arm32_DataProcessing_Logical_SetCPSR<false>(cpu, s, rd, res);
		break;}
	case 1: { // EOR
		op2 = Arm32_DataProcessing_GetShiftedOperand(cpu, i, op2, &rd15offset, true);
		u64 res = cpu->writeReg(rd, (cpu->reg[rn] + rd15offset*(rn==15)) ^ op2);
		Arm32_DataProcessing_Logical_SetCPSR<false>(cpu, s, rd, res);
		break;}
	case 2: { // SUB
		op2 = Arm32_DataProcessing_GetShiftedOperand(cpu, i, op2, &rd15offset, false);
		u32 a = (cpu->reg[rn] + rd15offset*(rn==15));
		u64 res = cpu->writeReg(rd, a - op2);
		Arm32_DataProcessing_Arithmetic_SetCPSR<true, false>(cpu, s, rd, a, op2, res);
		break;}
	case 3: { // RSB
		op2 = Arm32_DataProcessing_GetShiftedOperand(cpu, i, op2, &rd15offset, false);
		u32 a = (cpu->reg[rn] + rd15offset*(rn==15));
		u64 res = cpu->writeReg(rd, op2 - a);
		Arm32_DataProcessing_Arithmetic_SetCPSR<true, false>(cpu, s, rd, op2, a, res);
		break;}
	case 4: { // ADD
		op2 = Arm32_DataProcessing_GetShiftedOperand(cpu, i, op2, &rd15offset, false);
		u32 a = (cpu->reg[rn] + rd15offset*(rn==15));
		u64 res = cpu->writeReg(rd, a + op2);
		Arm32_DataProcessing_Arithmetic_SetCPSR<false, false>(cpu, s, rd, a, op2, res);
		//if (cpu->_lastPC == 0x0800'1f14)
		//	std::cout << "dingle: " << std::hex << res << std::dec << "\n";
		break;}
	case 5: { // ADC
		op2 = Arm32_DataProcessing_GetShiftedOperand(cpu, i, op2, &rd15offset, false);
		u32 a = (cpu->reg[rn] + rd15offset*(rn==15)) + cpu->cpsr.flagC;
		u64 res = cpu->writeReg(rd, a + op2);
		Arm32_DataProcessing_Arithmetic_SetCPSR<false, false>(cpu, s, rd, a, op2, res);
		break;}
	case 6: { // SBC
		op2 = Arm32_DataProcessing_GetShiftedOperand(cpu, i, op2, &rd15offset, false);
		u32 a = (cpu->reg[rn] + rd15offset*(rn==15)) + cpu->cpsr.flagC - 1;
		u64 res = cpu->writeReg(rd, a - op2);
		Arm32_DataProcessing_Arithmetic_SetCPSR<true, false>(cpu, s, rd, a, op2, res);
		break;}
	case 7: { // RSC
		op2 = Arm32_DataProcessing_GetShiftedOperand(cpu, i, op2, &rd15offset, false);
		u32 a = (cpu->reg[rn] + rd15offset*(rn==15)) - cpu->cpsr.flagC + 1;
		u64 res = cpu->writeReg(rd, op2 - a);
		Arm32_DataProcessing_Arithmetic_SetCPSR<true, false>(cpu, s, rd, op2, a, res);
		break;}
	case 8: { // TST
		op2 = Arm32_DataProcessing_GetShiftedOperand(cpu, i, op2, &rd15offset, true);
		u64 res = (cpu->reg[rn] + rd15offset*(rn==15)) & op2;
		Arm32_DataProcessing_Logical_SetCPSR<true>(cpu, s, rd, res);
		break;}
	case 9: { // TEQ
		op2 = Arm32_DataProcessing_GetShiftedOperand(cpu, i, op2, &rd15offset, true);
		u64 res = (cpu->reg[rn] + rd15offset*(rn==15)) ^ op2;
		Arm32_DataProcessing_Logical_SetCPSR<true>(cpu, s, rd, res);
		break;}
	case 10: { // CMP // OVERFLOW FLAG STILL BUGGED
		op2 = Arm32_DataProcessing_GetShiftedOperand(cpu, i, op2, &rd15offset, false);
		u32 a = (cpu->reg[rn] + rd15offset*(rn==15));
		u64 res = a - op2;
		Arm32_DataProcessing_Arithmetic_SetCPSR<true, true>(cpu, s, rd, a, op2, res);
		break;}
	case 11: { // CMN
		op2 = Arm32_DataProcessing_GetShiftedOperand(cpu, i, op2, &rd15offset, false);
		u32 a = (cpu->reg[rn] + rd15offset*(rn==15));
		u64 res = a + op2;
		Arm32_DataProcessing_Arithmetic_SetCPSR<true, true>(cpu, s, rd, a, op2, res);
		break;}
	case 12: { // ORR
		op2 = Arm32_DataProcessing_GetShiftedOperand(cpu, i, op2, &rd15offset, true);
		u64 res = cpu->writeReg(rd, (cpu->reg[rn] + rd15offset*(rn==15)) | op2);
		Arm32_DataProcessing_Logical_SetCPSR<false>(cpu, s, rd, res);
		break;}
	case 13: { // MOV
		op2 = Arm32_DataProcessing_GetShiftedOperand(cpu, i, op2, &rd15offset, true);
		cpu->writeReg(rd, op2);
		Arm32_DataProcessing_Logical_SetCPSR<false>(cpu, s, rd, op2);
		break;}
	case 14: { // BIC
		op2 = Arm32_DataProcessing_GetShiftedOperand(cpu, i, op2, &rd15offset, true);
		u64 res = cpu->writeReg(rd, (cpu->reg[rn] + rd15offset*(rn==15)) & (~op2));
		Arm32_DataProcessing_Logical_SetCPSR<false>(cpu, s, rd, res);
		break;}
	case 15: { // MVN
		op2 = Arm32_DataProcessing_GetShiftedOperand(cpu, i, op2, &rd15offset, true);
		cpu->writeReg(rd, ~op2);
		Arm32_DataProcessing_Logical_SetCPSR<false>(cpu, s, rd, ~op2);
		break;}
	}
}

// -- PSR Transfer Instructions -- //
void Arm32_MRS(Arm7* cpu, u32 instruction) {
	if (!evalConditionCode(cpu, CC((instruction >> 28) & 0xf)))
		return;
	bool from_spsr = (instruction >> 22) & 1;
	uint rd = (instruction >> 12) & 0xf;

	cpu->writeReg(rd, from_spsr ? cpu->readCurrentSPSR() : cpu->readCPSR());
}
void Arm32_MSR(Arm7* cpu, u32 instruction) {
	if (!evalConditionCode(cpu, CC((instruction >> 28) & 0xf)))
		return;
	bool to_spsr = (instruction >> 22) & 1;
	bool i = (instruction >> 25) & 1;
	bool f = (instruction >> 19) & 1;
	bool c = (instruction >> 16) & 1;
	u32 op = instruction & 0xfff;

	u32 rd15offset;
	op = Arm32_DataProcessing_GetShiftedOperand(cpu, i, op, &rd15offset, false);

	if (f) {
		if (to_spsr) {
			cpu->writeCurrentSPSR((op & 0xf000'0000) | (cpu->readCurrentSPSR() & 0x0fff'ffff)); // TODO: confirm what happens when current mode is SYS or USR. also can optimize this
		}
		else {
			cpu->cpsr.flagV = (op >> 28) & 1;
			cpu->cpsr.flagC = (op >> 29) & 1;
			cpu->cpsr.flagZ = (op >> 30) & 1;
			cpu->cpsr.flagN = (op >> 31) & 1;
		}
	}
	if (c && cpu->cpsr.mode != MODE_USER) { //"In non-privileged mode (user mode): only condition code bits of CPSR can be changed, control bits can't."
		if (to_spsr) {
			cpu->writeCurrentSPSR((op & 0b11011111) | (cpu->readCurrentSPSR() & ~(0b11011111))); // ^^^
		}
		else {
			cpu->setMode(op & 0b11111);
			// cpu->setThumbMode((op >> 5) & 1); // "The T-bit may not be changed; for THUMB/ARM switching use BX instruction."
			cpu->cpsr.FIQDisabled = (op >> 6) & 1;
			cpu->cpsr.IRQDisabled = (op >> 7) & 1;
		}
	}
}
//void Arm32_MSRFull(Arm7* cpu, u32 instruction) {
//	if (!evalConditionCode(cpu, CC((instruction >> 28) & 0xf)))
//		return;
//	bool p = (instruction >> 22) & 1;
//	uint rm = (instruction >> 0) & 0xf;
//
//	u32 val = cpu->readReg(rm);
//	if (p) {
//		cpu->writeCurrentSPSR(val);
//	}
//	else {
//		cpu->writeCPSR(val);
//	}
//}
//void Arm32_MSRPartial(Arm7* cpu, u32 instruction) {
//	if (!evalConditionCode(cpu, CC((instruction >> 28) & 0xf)))
//		return;
//	bool fullmsr = (instruction >> 16) & 1;
//	bool p = (instruction >> 22) & 1;
//	bool i = (instruction >> 25) & 1;
//	u32 op = instruction & 0xfff;
//
//	u32 rd15offset = 0;
//	op = Arm32_DataProcessing_GetShiftedOperand(cpu, i, op, &rd15offset, false); // TODO: make a seperate function because the behavior is different
//	if (p) {
//		cpu->writeCurrentSPSR((cpu->readCurrentSPSR() & 0x0fff'ffff) | (op & 0xf000'0000));
//	}
//	else {
//		cpu->cpsr.flagV = (op >> 28) & 1;
//		cpu->cpsr.flagC = (op >> 29) & 1;
//		cpu->cpsr.flagZ = (op >> 30) & 1;
//		cpu->cpsr.flagN = (op >> 31) & 1;
//	}
//}

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
void Arm32_Multiply(struct Arm7* cpu, u32 instruction) {
	if (!evalConditionCode(cpu, CC((instruction >> 28) & 0xf)))
		return;
	bool a = (instruction >> 21) & 1;
	bool s = (instruction >> 20) & 1;
	uint rd = (instruction >> 16) & 0xf;
	uint rn = (instruction >> 12) & 0xf;
	uint rs = (instruction >> 8) & 0xf;
	uint rm = (instruction >> 0) & 0xf;
	u32 res = (u64)(cpu->readReg(rm)) * (u64)(cpu->readReg(rs)) + (u64)(cpu->readReg(rn) * a);
	cpu->writeReg(rd, res);
	Arm32SetCPSR_MUL32(cpu, s, res);
}
// -- Long (64-bit) Multiplication -- (CC, A, S, RdHi, RdLo, Rs, Rm)
// TODO: convert the signed and unsigned into one function, its ok well make it a template later
void Arm32_MultiplyLong(struct Arm7* cpu, u32 instruction) {
	if (!evalConditionCode(cpu, CC((instruction >> 28) & 0xf)))
		return;
	bool u = (instruction >> 22) & 1;
	bool a = (instruction >> 21) & 1;
	bool s = (instruction >> 20) & 1;
	uint rdHi = (instruction >> 16) & 0xf;
	uint rdLo = (instruction >> 12) & 0xf;
	uint rs = (instruction >> 8) & 0xf;
	uint rm = (instruction >> 0) & 0xf;
	
	// Unsigned
	if (u) {
		u64 res = (u64)(cpu->readReg(rm)) * (u64)(cpu->readReg(rs)) + a * ((u64(cpu->readReg(rdHi)) << 32) | cpu->readReg(rdLo));
		cpu->writeReg(rdHi, res >> 32);
		cpu->writeReg(rdLo, res);
		Arm32SetCPSR_MUL64(cpu, s, res);
	}
	// Signed
	else {
		u64 res = (s64)((s32)(cpu->readReg(rm))) * (s64)((s32)(cpu->readReg(rs))) + a * ((u64(cpu->readReg(rdHi)) << 32) | cpu->readReg(rdLo));
		cpu->writeReg(rdHi, res >> 32);
		cpu->writeReg(rdLo, res);
		Arm32SetCPSR_MUL64(cpu, s, res);
	}
}
//void Arm32_MultiplyLong_Unsigned(struct Arm7* cpu, CC cc, bool a, bool s, uint rdHi, uint rdLo, uint rs, uint rm) {
//	u64 res = (u64)(cpu->readReg(rm)) * (u64)(cpu->readReg(rs)) + a * ((u64(cpu->readReg(rdHi)) << 32) | cpu->readReg(rdLo));
//	cpu->writeReg(rdHi, res >> 32);
//	cpu->writeReg(rdLo, res);
//	Arm32SetCPSR_MUL64(cpu, s, res);
//}
//void Arm32_MultiplyLong_Signed(struct Arm7* cpu, CC cc, bool a, bool s, uint rdHi, uint rdLo, uint rs, uint rm) {
//	u64 res = (s64)((s32)(cpu->readReg(rm))) * (s64)((s32)(cpu->readReg(rs))) + a * ((u64(cpu->readReg(rdHi)) << 32) | cpu->readReg(rdLo));
//	cpu->writeReg(rdHi, res >> 32);
//	cpu->writeReg(rdLo, res);
//	Arm32SetCPSR_MUL64(cpu, s, res);
//}

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

		assert(((off >> 4) & 1) == 0); // Shift amt can only be immediate (bit 4 must be 0);
		shift = (off >> 7) & 0b11111;

		// TODO: make this an inline function for readability
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
	if (!evalConditionCode(cpu, CC((instruction >> 28) & 0xf)))
		return;
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
	//if (i)
	//	std::cout << "dingle: " << std::hex << off << std::dec << "\n";
	if (!u)
		off = ~off + 1; // Negative
	u32 addr = base + off * p;

	// TODO: r15 reads should return +12 !!! just check eveyrthing
	// Load
	if (l) {
		if (b)
			cpu->writeReg(rd, cpu->read8(addr));
		else
			cpu->writeReg(rd, bitRotateRight(cpu->read32(addr & 0xffff'fffc), 32, (addr & 3)*8));
	}
	// Store
	else {
		if (b)
			cpu->write8(addr, u8(cpu->readReg(rd)));
		else
			cpu->write32(addr & 0xffff'fffc, cpu->readReg(rd));
	}

	if (w || p == 0)
		cpu->writeReg(rn, base + off);
}
void Arm32_HalfwordSignedDataTransfer(struct Arm7* cpu, u32 instruction) {
	if (!evalConditionCode(cpu, CC((instruction >> 28) & 0xf)))
		return;
	bool p = (instruction >> 24) & 1;
	bool u = (instruction >> 23) & 1;
	bool b = (instruction >> 22) & 1;
	bool w = (instruction >> 21) & 1;
	bool l = (instruction >> 20) & 1;
	uint rn = (instruction >> 16) & 0xf;
	uint rd = (instruction >> 12) & 0xf;
	bool s = (instruction >> 6) & 1;
	bool h = (instruction >> 5) & 1;
	uint op = (instruction >> 5) & 0b11;
	uint rm = (instruction >> 0) & 0xf;

	u32 base = cpu->readReg(rn);
	u32 off;
	
	if (b) {
		off = rm | (((instruction >> 8) & 0xf) << 4); // Immediate offset
	}
	else {
		off = cpu->readReg(rm); // Register offset
	}

	if (!u)
		off = ~off + 1; // Negative

	u32 addr = base + off * p;

	switch (op) {
	case 0b00: { // Swap Instruction
		printAndCrash("Oops! We decoded a HW/S Transfer instead of Swap!");
		break;
	}
	case 0b01: { // Unsigned Halfword transfer
		if (l)
			cpu->writeReg(rd, cpu->read16(addr & 0xffff'fffe)); // Halfword aligned, (unpredictable behavior when not)
		else
			cpu->write16(addr & 0xffff'fffe, cpu->readReg(rd)); // ^^^
		break;
	}
	case 0b10: { // Signed byte transfer
		if (l)
			cpu->writeReg(rd, cpu->readSigned8(addr));
		else
			std::cout << "Signed byte store?! PC: " << cpu->reg[15] << "\n"; assert(0);
		break;
	}
	case 0b11: { // Signed halfword transfer
		if (l)
			cpu->writeReg(rd, cpu->readSigned16(addr & 0xffff'fffe));
		else
			std::cout << "Signed halfword store?! PC: " << cpu->reg[15] << "\n"; assert(0);
		break;
	}
	}

	if (w || p == 0)
		cpu->writeReg(rn, base + off);
}
void Arm32_SingleDataSwap(struct Arm7* cpu, u32 instruction) {
	if (!evalConditionCode(cpu, CC((instruction >> 28) & 0xf)))
		return;
	bool b = (instruction >> 22) & 1;
	uint rn = (instruction >> 16) & 0xf;
	uint rd = (instruction >> 12) & 0xf;
	uint rm = (instruction >> 0) & 0xf;

	u32 addr = cpu->readReg(rn);
	if (b) {
		// Byte
		cpu->writeReg(rd, cpu->read8(addr));
		cpu->write8(addr, cpu->readReg(rm));
	}
	else {
		// Word
		cpu->writeReg(rd, bitRotateRight(cpu->read32(addr & 0xffff'fffc), 32, (addr & 3) * 8));
		cpu->write32(addr & 0xffff'fffc, cpu->readReg(rm));
	}
}

// -- Block Data Transfer Instructions -- // FIXME
void Arm32_BlockDataTransfer(Arm7* cpu, u32 instruction) {
	if (!evalConditionCode(cpu, CC((instruction >> 28) & 0xf)))
		return;
	bool p = (instruction >> 24) & 1;
	bool u = (instruction >> 23) & 1;
	bool s = (instruction >> 22) & 1;
	bool w = (instruction >> 21) & 1;
	bool l = (instruction >> 20) & 1;
	uint rn = (instruction >> 16) & 0xf;
	uint rlist = (instruction >> 0) & 0xffff;
	uint rcount = 0;
	for (int i = 0; i < 16; i++)
		rcount += (rlist >> i) & 1;

	if (s)
		print("FUNGUS"); // TODO: implement this shit

	u32 addr = cpu->readReg(rn) & 0xffff'fffc;
	if (w) {
		cpu->writeReg(rn, u ? addr + rcount * 4 : addr - rcount * 4);
	}
	if (!u) {
		addr -= rcount * 4;
		p = !p;
	}

	//s32 addrIncrement = u ? 4;
	if (cpu->canPrint()) std::cout << "block addr:\t" << std::hex << addr << std::dec << "\n";

	for (int i = 0; i < 16; i++) {
		if (((rlist >> i) & 1) == 0)
			continue;

		addr += 4 * p;
		if (l) {
			cpu->writeReg(i, cpu->read32(addr));
		}
		else {
			cpu->write32(addr, cpu->reg[i] + 8*(i==15));
		}
		addr += 4 * (!p);
	}
}

// -- Software Interrupt Instruction -- //
void Arm32_SoftwareInterrupt(Arm7* cpu, u32 instruction) {
	// Temporary HLE, BIOS not working atm
	switch (instruction & 0xff'ffff) {
	case 0x06'0000: { // DIV
		s32 r0 = cpu->reg[0];
		s32 r1 = cpu->reg[1];

		cpu->reg[0] = u32(r0 / r1);
		cpu->reg[1] = u32(r0 % r1);
		cpu->reg[3] = cpu->reg[0] / cpu->reg[1]; // unsigned
		return;
		break;
	}
	}

	//cpu->setMode(MODE_SVC);
	////cpu->copyCPSRToSPSR();
	//cpu->writeReg(14, cpu->readReg(15));
	//cpu->writeReg(15, 0x08);
}

// -- Undefined Instruction -- //
void Arm32_Undefined(Arm7* cpu, u32 instruction) {
	std::cout << "undefined reached at pc: " << std::hex << cpu->reg[15] << "\n";
	assert(0);
}
void Arm32_DEBUG_NOOP(Arm7* cpu, u32 instruction) {
}

// Fetching and Decoding
u32 Arm32_FetchInstruction(Arm7* cpu) {
	//if (cpu->reg[15] == 0x0800'00f0) // BREAKPOINT
	//	cpu->PRINTSTATE();
	if (cpu->reg[15] < 0x0800'0000 /*&& cpu->reg[15] >= 0x0000'4000*/) { // OOB CHECK
		std::cout << "\nR15:\t" << std::hex << cpu->reg[15] << ", exe: " << std::dec << cpu->_executionsRan << ", r12: " << cpu->reg[12] << "\n";
		assert(0);
	}

	cpu->reg[15] &= 0xffff'fffc;
	if (cpu->canPrint()) std::cout << "\nR15:\t" << std::hex << cpu->reg[15] << std::dec << "\n";
	u32 instruction = cpu->read32(cpu->reg[15]);
	cpu->reg[15] += 4;
	return instruction;
}

// Note, this function interprets the instruction as big-endian
typedef void (*ArmInstructionFunc)(struct Arm7*, u32);
ArmInstructionFunc Arm32_Decode(Arm7* cpu, u32 instruction) {
	switch ((instruction >> 26) & 0b11) {
	case 0b00: {
		u32 bits543210 = (instruction >> 20) & 0b111111; // TODO: Remove the and later
		u32 bits7654 = (instruction >> 4) & 0b1111;

		// TODO: can optimize common cases for bits7654, just make sure it works first
		if ((bits543210 & 0b111100) == 0b000000 && bits7654 == 0b1001) {// Needs arguments to be rewritten
			if (cpu->canPrint()) std::cout << "Multiply:\t\t" << std::hex << instruction << std::dec << "\n";
			return &Arm32_Multiply;
		}
		if ((bits543210 & 0b111000) == 0b001000 && bits7654 == 0b1001) {// Needs to be implemented
			if (cpu->canPrint()) std::cout << "Multiply Long:\t\t" << std::hex << instruction << std::dec << "\n";
			return &Arm32_MultiplyLong;
		}
		if ((bits543210 & 0b111011) == 0b010000 && bits7654 == 0b1001) { // ^^^
			if (cpu->canPrint()) std::cout << "Single Data Swap:\t" << std::hex << instruction << std::dec << "\n";
			return &Arm32_SingleDataSwap;
		}
		if ((bits543210 & 0b100000) == 0b000000 && (bits7654 & 0b1001) == 0b1001) {
			if (cpu->canPrint()) std::cout << "HW/S Data Transfer:\t" << std::hex << instruction << std::dec << "\n";
			return &Arm32_HalfwordSignedDataTransfer;
		}
		if ((bits543210 & 0b111011) == 0b010000 && bits7654 == 0b0000) {
			/*if (cpu->canPrint())*/ std::cout << "MRS:\t" << std::hex << instruction << std::dec << "\n";
			return &Arm32_MRS;
		}
		if (
			((bits543210 & 0b111011) == 0b010010 && bits7654 == 0b0000) ||	// Register ; TODO: optimize this fuckass thing here
			((bits543210 & 0b111011) == 0b110010)							// Immediate
		) {
			/*if (cpu->canPrint())*/ std::cout << "MSR:\t" << std::hex << instruction << std::dec << "\n";
			return &Arm32_MSR;
		}
		//if ((bits543210 & 0b111011) == 0b010010 && bits7654 == 0b0000) {
		//	if (cpu->canPrint()) std::cout << "MSR Full:\t" << std::hex << instruction << std::dec << "\n";
		//	return &Arm32_MSRFull;
		//}
		//if ((bits543210 & 0b111011) == 0b110010) {
		//	if (cpu->canPrint()) std::cout << "MSR Partial:\t" << std::hex << instruction << std::dec << "\n";
		//	return &Arm32_MSRPartial;
		//}
		if ((bits543210 & 0b111111) == 0b010010 && bits7654 == 0b0001) {
			if (cpu->canPrint()) std::cout << "Branch and Exchange:\t" << std::hex << instruction << std::dec << "\n";
			return &Arm32_BranchAndExchange;
		}
		if ((bits543210 & 0b111011) == 0b110000) {
			if (cpu->canPrint()) std::cout << "Invalid instruction! \nins: " << std::hex << instruction << "\nbits543210: " << std::dec << std::bitset<6>(bits543210) << "\tbits7654: " << std::bitset<4>(bits7654) << "\n\n";
			assert(0);
		}
		if (cpu->canPrint()) std::cout << "Data Processing:\t" << std::hex << instruction << std::dec << "\n";
		return &Arm32_DataProcessing;
		break;
	}
	case 0b01: {
		u32 bits543210 = (instruction >> 20) & 0b111111;
		u32 bits7654 = (instruction >> 4) & 0b1111;

		if ((bits543210 & 0b100000) == 0b100000 && (bits7654 & 1) == 1) {
			std::cout << "Undefined Instruction:\t" << std::hex << instruction << std::dec << "\n";
			return &Arm32_Undefined;
		}
		else {
			if (cpu->canPrint()) std::cout << "Single Data Transfer:\t" << std::hex << instruction << std::dec << "\n";
			return &Arm32_SingleDataTransfer;
		}
		break;
	}
	case 0b10: {
		u32 bits543210 = (instruction >> 20) & 0b111111;
		u32 bits7654 = (instruction >> 4) & 0b1111;

		if ((bits543210 & 0b100000) == 0b100000) {
			//std::cout << "\nR15:\t" << std::hex << cpu->reg[15] << std::dec << "\n";
			if (cpu->canPrint()) std::cout << "Branch and Link:\t" << std::hex << instruction << std::dec << "\n";
			return &Arm32_BranchAndLink;
		}
		else {
			if (cpu->canPrint()) std::cout << "Block Data Transfer:\t" << std::hex << instruction << std::dec << "\n";
			return &Arm32_BlockDataTransfer;
			//assert(0);
		}
		break;
	}
	case 0b11: {
		u32 bits543210 = (instruction >> 20) & 0b111111;
		u32 bits7654 = (instruction >> 4) & 0b1111;

		std::cout << "Group 11 instruction:\t" << std::hex << instruction << std::dec << "\n";
		if ((bits543210 & 0b110000) == 0b110000) {
			std::cout << "R15:\t" << std::hex << cpu->reg[15] << std::dec << "\n";
			std::cout << "SWI; r0 is: " << std::hex << cpu->reg[12] << std::dec << "\n";
			return &Arm32_SoftwareInterrupt;
		}
		//assert(0);
		return &Arm32_DEBUG_NOOP;
		break;
	}
	}
}

void TEST_ARM32DECODE() {
	//assert(Arm32_Decode(0xe12fff10) == &Arm32_BranchAndExchange);	// bx r0
	//assert(Arm32_Decode(0xe0a00001) == &Arm32_DataProcessing);		// adc r0, r1
	//assert(Arm32_Decode(0xe0000190) == &Arm32_Multiply);			// mul r0, r1
	//assert(Arm32_Decode(0xe0810392) == &Arm32_MultiplyLong);		// umull, r0, r1, r2, r3
	//assert(Arm32_Decode(0xe1010092) == &Arm32_SingleDataSwap);		// swp, r0, r2, [r1]
	//assert(Arm32_Decode(0xe6910002) == &Arm32_SingleDataTransfer);	// ldr r0, [r1], r2
	//assert(Arm32_Decode(0xe6910012) == &Arm32_Undefined);			// undefined instruction
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