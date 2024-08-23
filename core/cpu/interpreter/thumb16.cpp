// TODO: later on, make the Arm Instruction functions take in parameters seperately, so that we dont have to extract everything twice, instead just pass them here in the thumb functions, and extract them seperately in the arm decoding functions
// This is for later, once we make sure that everything is working

void Thumb16_MoveShiftedRegister(Arm7* cpu, u16 instruction) {
	uint op = (instruction >> 11) & 0b11;
	u32 off = (instruction >> 6) & 0b11111;
	uint rs = (instruction >> 3) & 0b111;
	uint rd = (instruction >> 0) & 0b111;
	if (op == 3) {
		print("Invalid Thumb MoveShiftedRegister Instruction");
		cpu->PRINTSTATE();
	}

	u32 inst = 0b1110'00'0'1101'1'0000'0000'000000000000; // MOVS Rd, Rs, () base
	inst |= rd << 12;
	inst |= off << 7;
	inst |= op << 5;
	inst |= rs << 0;
	Arm32_DataProcessing<true>(cpu, inst);
	return;

	//u32 op2 = Arm32_DataProcessing_GetShiftedOperand(cpu, 0, ((off << 7) | (op << 5) | rs), true);

	//cpu->reg[rd] = op2; // NOTE: were accessing the registers directly here because the special case of reading/writing to r15 can never happen cuz max we can address is r7
	//Arm32_DataProcessing_Logical_SetCPSR(cpu, true, rd, op2); // TODO: can optimize and remove unneccassary checks later, same for GetShiftedOperand

	// TODO: if using this method below, dont forget to implement the flag changes
	//switch (op) {
	//case 0: {
	//	cpu->reg[rd] = cpu->reg[rs] << off; // NOTE: were accessing the registers directly here because the special case of reading/writing to r15 can never happen cuz max we can address is r7
	//	break;
	//}
	//case 1: {
	//	cpu->reg[rd] = cpu->reg[rs] >> off;
	//	break;
	//}
	//case 2: {
	//	cpu->reg[rd] = bitSignedShiftRight(cpu->reg[rs], 32, off);
	//	break;
	//}
	//case 3: {
	//	print("INVALID THUMB INSTRUCTION FORMAT");
	//	cpu->PRINTSTATE();
	//	break;
	//}
	//}
}

void Thumb16_AddSubtract(Arm7* cpu, u16 instruction) {
	bool i = (instruction >> 10) & 1;
	bool sub = (instruction >> 9) & 1;
	uint rn = (instruction >> 6) & 0b111;
	uint rs = (instruction >> 3) & 0b111;
	uint rd = (instruction >> 0) & 0b111;

	u32 inst = 0b1110'00'0'0000'1'0000'0000'000000000000; // (ADD/SUB)S Rd, Rs, () base
	inst |= 1 << (23 - sub);
	inst |= i << 25;
	inst |= rs << 16;
	inst |= rd << 12;
	inst |= rn;
	Arm32_DataProcessing<true>(cpu, inst);
	return;

	//u32 op2 = i ? rn : cpu->reg[rn];
	//if (sub)
	//	op2 = ~op2 + 1; // negate

	//u64 res = cpu->reg[rs] + op2;
	//cpu->reg[rd] = res;
	//Arm32_DataProcessing_Arithmetic_SetCPSR(cpu, true, 0, cpu->reg[rs], op2, res);
}

void Thumb16_MovCmpAddSubImmediate(Arm7* cpu, u16 instruction) {
	uint op = (instruction >> 11) & 0b11;
	uint rd = (instruction >> 8) & 0b111;
	u32 off8 = (instruction >> 0) & 0xff;

	u32 inst = 0b1110'00'1'0000'1'0000'0000'000000000000; // ()S Rd, Rs, # base
	//inst |= (1 << (23 - (op & 1))) | ((0b1000 << 23) * (op >= 0b10)) | ((0b0001 << 23) * (op == 0b11)); // TODO: recheck this
	switch (op) { // Safe method
	case 0b00: op = 0b1101; break;
	case 0b01: op = 0b1010; break;
	case 0b10: op = 0b0100; break;
	case 0b11: op = 0b0010; break;
	}
	inst |= op << 21;
	inst |= rd << 16;
	inst |= rd << 12;
	inst |= off8;
	Arm32_DataProcessing<true>(cpu, inst);
	return;

	//switch (op) {
	//case 0: { // MOVS
	//	cpu->reg[rd] = op2;
	//	Arm32_DataProcessing_Logical_SetCPSR(cpu, true, rd, op2);
	//	break;
	//}
	//case 1: { // CMP
	//	op2 = ~op2 + 1; // negate
	//	u64 res = cpu->reg[rd] + op2;
	//	Arm32_DataProcessing_Arithmetic_SetCPSR(cpu, true, 0, cpu->reg[rd], op2, res);
	//	break;
	//}
	//case 3: {
	//	op2 = ~op2 + 1;
	//	// dont break
	//}
	//case 2: { // ADDS
	//	u64 res = cpu->reg[rd] + op2;
	//	cpu->reg[rd] = res;
	//	Arm32_DataProcessing_Arithmetic_SetCPSR(cpu, true, 0, cpu->reg[rd], op2, res);
	//	break;
	//}
	//}
}

void Thumb16_ALUOperations(Arm7* cpu, u16 instruction) {
	uint op = (instruction >> 6) & 0xf;
	uint rs = (instruction >> 3) & 0b111;
	uint rd = (instruction >> 0) & 0b111;

	// TODO: finish this shit
	switch (op) {
	case 0b0010: { // LSL
		u32 inst = 0b1110'00'0'1101'1'0000'0000'000000000000;
		inst |= rd << 12;
		inst |= rd;
		inst |= 1 << 4;
		inst |= 0b00 << 5;
		inst |= rs << 8;
		Arm32_DataProcessing<true>(cpu, inst);
		break;
	}
	case 0b0011: { // LSR
		u32 inst = 0b1110'00'0'1101'1'0000'0000'000000000000;
		inst |= rd << 12;
		inst |= rd;
		inst |= 1 << 4;
		inst |= 0b01 << 5;
		inst |= rs << 8;
		Arm32_DataProcessing<true>(cpu, inst);
		break;
	}
	case 0b0100: { // ASR
		u32 inst = 0b1110'00'0'1101'1'0000'0000'000000000000;
		inst |= rd << 12;
		inst |= rd;
		inst |= 1 << 4;
		inst |= 0b10 << 5;
		inst |= rs << 8;
		Arm32_DataProcessing<true>(cpu, inst);
		break;
	}
	case 0b0111: { // ROR
		u32 inst = 0b1110'00'0'1101'1'0000'0000'000000000000;
		inst |= rd << 12;
		inst |= rd;
		inst |= 1 << 4;
		inst |= 0b11 << 5;
		inst |= rs << 8;
		Arm32_DataProcessing<true>(cpu, inst);
		break;
	}
	case 0b1001: { // NEG
		u32 inst = 0b1110'00'1'0011'1'0000'0000'000000000000; // RSBS rd, rs, #0
		inst |= rs << 16;
		inst |= rd << 12;
		Arm32_DataProcessing<true>(cpu, inst);
		break;
	}
	case 0b1101: { // MUL
		u32 inst = 0b1110'000000'0'1'0000'0000'0000'1001'0000; // MULS rd, rm, rs
		inst |= rd << 16;
		inst |= rd << 8;
		inst |= rs;
		Arm32_Multiply(cpu, inst);
		break;
	}
	default: {
		u32 inst = 0b1110'00'0'0000'1'0000'0000'000000000000; // ()S Rd, Rd, Rs
		inst |= op << 21;
		inst |= rd << 16;
		inst |= rd << 12;
		inst |= rs;
		Arm32_DataProcessing<true>(cpu, inst);
		break;
	}
	}
}

void Thumb16_HiRegisterOperations(Arm7* cpu, u16 instruction) {
	uint op = (instruction >> 8) & 0b11;
	bool h1 = (instruction >> 7) & 1;
	bool h2 = (instruction >> 6) & 1;
	uint rs = (instruction >> 3) & 0b111;
	uint rd = instruction & 0b111;

	if (h1)
		rd |= 0b1000;
	if (h2)
		rs |= 0b1000;

	switch (op) {
	case 0b00: { // ADD
		u32 inst = 0b1110'00'0'0100'0'0000'0000'000000000000;
		inst |= rd << 16;
		inst |= rd << 12;
		inst |= rs;
		Arm32_DataProcessing<true>(cpu, inst);
		break;
	}
	case 0b01: { // CMP
		u32 inst = 0b1110'00'0'1010'1'0000'0000'000000000000;
		inst |= rd << 16;
		inst |= rs;
		Arm32_DataProcessing<true>(cpu, inst);
		break;
	}
	case 0b10: { // MOV
		u32 inst = 0b1110'00'0'1101'0'0000'0000'000000000000;
		inst |= rd << 12;
		inst |= rs;
		Arm32_DataProcessing<true>(cpu, inst);
		break;
	}
	case 0b11: { // BX
		u32 inst = 0b1110'0001'0010'1111'1111'1111'0001'0000;
		inst |= rs;
		Arm32_BranchAndExchange<true>(cpu, inst);
		break;
	}
	}
}

// PC-Relative Load
void Thumb16_PCRelativeLoad(Arm7* cpu, u16 instruction) {
	uint rd = (instruction >> 8) & 0b111;
	u32 word8 = instruction & 0xff;

	u32 inst = 0b1110'01'0'1'1'0'0'1'1111'0000'000000000000; // LDR Rd, [R15, #] // CONFIRM: no writeback, right?
	inst |= rd << 12;
	inst |= word8 << 2;
	Arm32_SingleDataTransfer<true>(cpu, inst);
}

// Load/Store with Register Offset
void Thumb16_LoadStoreWithRegisterOffset(Arm7* cpu, u16 instruction) {
	bool l = (instruction >> 11) & 1;
	bool b = (instruction >> 10) & 1;
	uint ro = (instruction >> 6) & 0b111;
	uint rb = (instruction >> 3) & 0b111;
	uint rd = (instruction >> 0) & 0b111;

	u32 inst = 0b1110'01'1'1'1'0'0'0'0000'0000'000000000000; // LDR/STR Rd, [Rb, Ro]
	inst |= b << 22;
	inst |= l << 20;
	inst |= rb << 16;
	inst |= rd << 12;
	inst |= ro;
	Arm32_SingleDataTransfer<true>(cpu, inst);
}

// Load/Store Sign Extended Byte/Halfword
void Thumb16_LoadStoreSignExtendedByteHalfword(Arm7* cpu, u16 instruction) {
	uint ro = (instruction >> 6) & 0b111;
	uint rb = (instruction >> 3) & 0b111;
	uint rd = (instruction >> 0) & 0b111;
	uint op = (((instruction >> 10) & 1) << 1) | ((instruction >> 11) & 1); // Why did they encode the bits upside down bruh, TODO: can be a lil optimized later albeit unreadable

	bool l = true;
	// Handle invalid SWP opcode, turn it into STRH
	l &= op != 0;
	op += op == 0;

	u32 inst = 0b1110'000'1'1'0'0'0'0000'0000'0000'1'00'1'0000; // LDRH/SB/SH Rd, [Rb, Ro]
	inst |= l << 20;
	inst |= rb << 16;
	inst |= rd << 12;
	inst |= ro;
	inst |= op << 5;
	Arm32_HalfwordSignedDataTransfer(cpu, inst);
}

// Load/Store With Immediate Offset
void Thumb16_LoadStoreWithImmediateOffset(Arm7* cpu, u16 instruction) {
	bool b = (instruction >> 12) & 1;
	bool l = (instruction >> 11) & 1;
	uint rb = (instruction >> 3) & 0b111;
	uint rd = (instruction >> 0) & 0b111;
	u32 off5 = (instruction >> 6) & 0b11111;

	u32 inst = 0b1110'01'0'1'1'0'0'0'0000'0000'000000000000; // LDR/STR Rd, [Rb, #]
	inst |= b << 22;
	inst |= l << 20;
	inst |= rb << 16;
	inst |= rd << 12;
	inst |= off5 << (2 * !b);
	Arm32_SingleDataTransfer<true>(cpu, inst);
}

// Load/Store Halfword
void Thumb16_LoadStoreHalfword(Arm7* cpu, u16 instruction) {
	bool l = (instruction >> 11) & 1;
	uint rb = (instruction >> 3) & 0b111;
	uint rd = (instruction >> 0) & 0b111;
	u32 off5 = ((instruction >> 6) & 0b11111) << 1;

	u32 inst = 0b1110'000'1'1'1'0'0'0000'0000'0000'1'01'1'0000; // LDRH/STRH, [Rb, Ro]
	inst |= l << 20;
	inst |= rb << 16;
	inst |= rd << 12;
	inst |= off5 & 0xf;
	inst |= (off5 >> 4) << 8;
	Arm32_HalfwordSignedDataTransfer(cpu, inst);
}

// SP-Relative Load/Store
void Thumb16_SPRelativeLoadStore(Arm7* cpu, u16 instruction) {
	bool l = (instruction >> 11) & 1;
	uint rd = (instruction >> 8) & 0b111;
	u32 off8 = instruction & 0xff;

	u32 inst = 0b1110'01'0'1'1'0'0'0'0000'0000'000000000000; // LDR/STR Rd, [Rb, #]
	inst |= l << 20;
	inst |= 13 << 16;
	inst |= rd << 12;
	inst |= off8 << 2;
	Arm32_SingleDataTransfer<true>(cpu, inst);
	// DOUBLE CHECK THIS, i forgot if i finished tis
}

// Load Address
void Thumb16_LoadAddress(Arm7* cpu, u16 instruction) {
	bool sp = (instruction >> 11) & 1;
	uint rd = (instruction >> 8) & 0b111;
	u32 word8 = instruction & 0xff;

	// Direct method; Not converting to ARM because this fuckass instruction sets bit 1 of readed PC to 0, dont wanna make my ARM ALU instruction a templated hell
	u32 a;
	if (!sp) {
		a = (cpu->reg[15] + 2) & ~2;
		a &= ~2;
	}
	else {
		a = cpu->reg[13];
	}
	u64 res = cpu->reg[rd] = a + (word8 << 2); // Direct write because PC can never be accessed
	//Arm32_DataProcessing_Arithmetic_SetCPSR<false, false>(cpu, s, rd, a, op2, res);

	// Translated method; doesnt clear bit 1 of readed PC
	//u32 inst = 0b1110'00'1'0100'0'0000'0000'000000000000; // ADD rd, (), # base
	//inst |= (15 - (sp*2)) << 16;
	//inst |= rd << 12;
	//inst |= word8;
	//inst |= 15 << 8; // ROR 30; shift left by 2
	//Arm32_DataProcessing<true>(cpu, inst);
}

// Add Offset to Stack Pointer
void Thumb16_AddOffsetToStackPointer(Arm7* cpu, u16 instruction) {
	bool s = (instruction >> 7) & 1;
	u32 word7 = instruction & 0x7f;

	u32 inst = 0b1110'00'1'0000'0'1101'1101'000000000000; // () R13, R13, #
	inst |= (0b0100 >> (int)s) << 21;
	inst |= word7;
	inst |= 15 << 8; // ROR by 30; word7 shifted left 2 bits
	Arm32_DataProcessing<true>(cpu, inst);
}

// Push/Pop Registers
void Thumb16_PushPopRegisters(Arm7* cpu, u16 instruction) {
	bool l = (instruction >> 11) & 1;
	bool r = (instruction >> 8) & 1;
	u32 rlist = instruction & 0xff;

	bool p = !l;
	bool u = l;

	u32 inst = 0b1110'100'0'0'0'1'0'1101'0000000000000000; // LDM/STM r13!, {}
	inst |= p << 24;
	inst |= u << 23;
	inst |= l << 20;
	inst |= rlist;
	inst |= r << (14 + l);
	Arm32_BlockDataTransfer<true>(cpu, inst);
}

// Multiple Load/Store
void Thumb16_MultipleLoadStore(Arm7* cpu, u16 instruction) {
	bool l = (instruction >> 11) & 1;
	u32 rb = (instruction >> 8) & 0b111;
	u32 rlist = instruction & 0xff;

	u32 inst = 0b1110'100'0'1'0'1'0'0000'0000000000000000; // LDM/STM rb!, {rlist}
	inst |= l << 20;
	inst |= rb << 16;
	inst |= rlist;
	Arm32_BlockDataTransfer<true>(cpu, inst);
}

// Conditional Branch
void Thumb16_ConditionalBranch(Arm7* cpu, u16 instruction) {
	u32 cond = (instruction >> 8) & 0xf;
	u32 off8 = instruction & 0xff;

	//u32 inst = 0b0000'101'0'000000000000000000000000; // B()
	//inst |= cond << 28;

	if (!evalConditionCode(cpu, CC(cond)))
		return;

	off8 |= 0xffff'ff00 * (off8 >> 7);
	off8 <<= 1; // Now 9 bits
	s32 soff = s32(off8);

	u32 pc = cpu->reg[15];
	//if (l)
	//	cpu->writeReg(14, pc);
	cpu->writeReg(15, pc + 2 + soff);
}

// Software Interrupt
void Thumb16_SoftwareInterrupt(Arm7* cpu, u16 instruction) {
	u32 inst = 0b1110'1111'000000000000000000000000 | (instruction & 0xff);
	Arm32_SoftwareInterrupt(cpu, inst);
}

// Unconditional branch
void Thumb16_UnconditionalBranch(Arm7* cpu, u16 instruction) {
	u32 off11 = instruction & 0x7ff;

	off11 |= ~(u32)0x7ff * (off11 >> 10);
	off11 <<= 1; // Now 12 bits
	s32 soff = s32(off11);

	u32 pc = cpu->reg[15];
	//if (l)
	//	cpu->writeReg(14, pc);
	cpu->writeReg(15, pc + 2 + soff);
}

// Long Branch With Link
void Thumb16_LongBranchWithLink(Arm7* cpu, u16 instruction) {
	bool h = (instruction >> 11) & 1;
	u32 off = instruction & 0x7ff;

	// Instruction part 1
	if (!h) {
		off |= ~(u32)0x7ff * (off >> 10);
		cpu->writeReg(14, cpu->reg[15] + 2 + (off << 12));
	}
	// Instruction part 2
	else {
		u32 addrAfterInst = cpu->reg[15];
		cpu->writeReg(15, cpu->reg[14] + (off << 1));
		cpu->writeReg(14, addrAfterInst | 1);
	}
}

// DEBUG
void Thumb16_DEBUGNOOP(Arm7* cpu, u16 instruction) {

}

// Fetching and Decoding
u16 Thumb16_FetchInstruction(Arm7* cpu) {
	cpu->BEFOREFETCH();

	cpu->reg[15] &= 0xffff'fffe;
	if (cpu->canPrint()) std::cout << "\nR15:\t" << std::hex << cpu->reg[15] << std::dec << "\n";
	u16 instruction = cpu->read16(cpu->reg[15]);
	cpu->reg[15] += 2;
	return instruction;
}

typedef void (*ThumbInstructionFunc)(struct Arm7*, u16);
ThumbInstructionFunc Thumb16_Decode(Arm7* cpu, u16 instruction) {
	u32 bits5432109876 = instruction >> 6;

	if ((bits5432109876 & 0b1111100000) == 0b0001100000) {
		if (cpu->canPrint()) std::cout << "Thumb16_AddSubtract:\t" << std::hex << instruction << std::dec << "\n";
		return &Thumb16_AddSubtract;
	}
	if ((bits5432109876 & 0b1110000000) == 0b0000000000) {
		if (cpu->canPrint()) std::cout << "Thumb16_MoveShiftedRegister:\t" << std::hex << instruction << std::dec << "\n";
		return &Thumb16_MoveShiftedRegister;
	}
	if ((bits5432109876 & 0b1110000000) == 0b0010000000) {
		if (cpu->canPrint()) std::cout << "Thumb16_MovCmpAddSubImmediate:\t" << std::hex << instruction << std::dec << "\n";
		return &Thumb16_MovCmpAddSubImmediate;
	}
	if ((bits5432109876 & 0b1111110000) == 0b0100000000) {
		if (cpu->canPrint()) std::cout << "Thumb16_ALUOperations:\t" << std::hex << instruction << std::dec << "\n";
		return &Thumb16_ALUOperations;
	}
	if ((bits5432109876 & 0b1111110000) == 0b0100010000) {
		if (cpu->canPrint()) std::cout << "Thumb16_HiRegisterOperations:\t" << std::hex << instruction << std::dec << "\n";
		return &Thumb16_HiRegisterOperations;
	}
	if ((bits5432109876 & 0b1111100000) == 0b0100100000) {
		if (cpu->canPrint()) std::cout << "Thumb16_PCRelativeLoad:\t" << std::hex << instruction << std::dec << "\n";
		return &Thumb16_PCRelativeLoad;
	}
	if ((bits5432109876 & 0b1111001000) == 0b0101000000) {
		if (cpu->canPrint()) std::cout << "Thumb16_LoadStoreWithRegisterOffset:\t" << std::hex << instruction << std::dec << "\n";
		return &Thumb16_LoadStoreWithRegisterOffset;
	}
	if ((bits5432109876 & 0b1111001000) == 0b0101001000) {
		if (cpu->canPrint()) std::cout << "Thumb16_LoadStoreSignExtendedByteHalfword:\t" << std::hex << instruction << std::dec << "\n";
		return &Thumb16_LoadStoreSignExtendedByteHalfword;
	}
	if ((bits5432109876 & 0b1110000000) == 0b0110000000) {
		if (cpu->canPrint()) std::cout << "Thumb16_LoadStoreWithImmediateOffset:\t" << std::hex << instruction << std::dec << "\n";
		return &Thumb16_LoadStoreWithImmediateOffset;
	}
	if ((bits5432109876 & 0b1111000000) == 0b1000000000) {
		if (cpu->canPrint()) std::cout << "Thumb16_LoadStoreHalfword:\t" << std::hex << instruction << std::dec << "\n";
		return &Thumb16_LoadStoreHalfword;
	}
	if ((bits5432109876 & 0b1111000000) == 0b1001000000) {
		if (cpu->canPrint()) std::cout << "Thumb16_SPRelativeLoadStore:\t" << std::hex << instruction << std::dec << "\n";
		return &Thumb16_SPRelativeLoadStore;
	}
	if ((bits5432109876 & 0b1111000000) == 0b1010000000) {
		if (cpu->canPrint()) std::cout << "Thumb16_LoadAddress:\t" << std::hex << instruction << std::dec << "\n";
		return &Thumb16_LoadAddress;
	}
	if ((bits5432109876 & 0b1111111100) == 0b1011000000) {
		if (cpu->canPrint()) std::cout << "Thumb16_AddOffsetToStackPointer:\t" << std::hex << instruction << std::dec << "\n";
		return &Thumb16_AddOffsetToStackPointer;
	}
	if ((bits5432109876 & 0b1111011000) == 0b1011010000) {
		if (cpu->canPrint()) std::cout << "Thumb16_PushPopRegisters:\t" << std::hex << instruction << std::dec << "\n";
		return &Thumb16_PushPopRegisters;
	}
	if ((bits5432109876 & 0b1111000000) == 0b1100000000) {
		if (cpu->canPrint()) std::cout << "Thumb16_MultipleLoadStore:\t" << std::hex << instruction << std::dec << "\n";
		return &Thumb16_MultipleLoadStore;
	}
	if ((bits5432109876 & 0b1111111100) == 0b1101111100) {
		if (cpu->canPrint()) std::cout << "Thumb16_SoftwareInterrupt:\t" << std::hex << instruction << std::dec << "\n";
		return &Thumb16_SoftwareInterrupt;
	}
	if ((bits5432109876 & 0b1111111100) == 0b1101111000) {
		std::cout << "UNDEFINED THUMB INS:\t" << std::hex << instruction << std::dec << "\n";
		cpu->PRINTSTATE();
		return &Thumb16_SoftwareInterrupt;
	}
	if ((bits5432109876 & 0b1111000000) == 0b1101000000) {
		if (cpu->canPrint()) std::cout << "Thumb16_ConditionalBranch:\t" << std::hex << instruction << std::dec << "\n";
		return &Thumb16_ConditionalBranch;
	}
	if ((bits5432109876 & 0b1111100000) == 0b1110000000) {
		if (cpu->canPrint()) std::cout << "Thumb16_UnconditionalBranch:\t" << std::hex << instruction << std::dec << "\n";
		return &Thumb16_UnconditionalBranch;
	}
	if ((bits5432109876 & 0b1111000000) == 0b1111000000) {
		if (cpu->canPrint()) std::cout << "Thumb16_LongBranchWithLink:\t" << std::hex << instruction << std::dec << "\n";
		return &Thumb16_LongBranchWithLink;
	}

	std::cout << "Unimplemented THUMB instruction:\t" << std::hex << instruction << std::dec << "\n";
	//cpu->PRINTSTATE();
	return &Thumb16_DEBUGNOOP;
}