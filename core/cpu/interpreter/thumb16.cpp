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
	Arm32_DataProcessing(cpu, inst);
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
	Arm32_DataProcessing(cpu, inst);
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
	inst |= (1 << (23 - (op & 1))) | ((0b1000 << 23) * (op >= 0b10)) | ((0b0001 << 23) * (op == 0b11)); // TODO: recheck this
	inst |= rd << 16;
	inst |= rd << 12;
	inst |= off8;
	Arm32_DataProcessing(cpu, inst);
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

	u32 inst = 0b1110'00'1'0000'1'0000'0000'000000000000; // ()S Rd, Rs, # base
	switch (op) {
	case 0b0010: { // LSL
		break;
	}
	case 0b0011: { // LSR
		break;
	}
	case 0b0100: { // ASR
		break;
	}
	case 0b0111: { // ROR
		break;
	}
	case 0b1001: { // NEG
		break;
	}
	case 0b1101: { // MUL
		break;
	}
	default: {
		inst |= op << 21;
		inst |= rd << 16;
		inst |= rd << 12;
		inst |= rs;
		Arm32_DataProcessing(cpu, inst);
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
		rs |= 0b1000;
	if (h2)
		rs |= 0b1000;

	switch (op) {
	case 0b00: { // ADD
		u32 inst = 0b1110'00'0'0100'0'0000'0000'000000000000;
		inst |= rd << 16;
		inst |= rd << 12;
		inst |= rs;
		Arm32_DataProcessing(cpu, inst);
		break;
	}
	case 0b01: { // CMP
		u32 inst = 0b1110'00'0'1010'1'0000'0000'000000000000;
		inst |= rd << 16;
		inst |= rs;
		Arm32_DataProcessing(cpu, inst);
		break;
	}
	case 0b10: { // MOV
		u32 inst = 0b1110'00'0'1101'0'0000'0000'000000000000;
		inst |= rd << 12;
		inst |= rs;
		Arm32_DataProcessing(cpu, inst);
		break;
	}
	case 0b11: { // BX
		u32 inst = 0b1110'0001'0010'1111'1111'1111'0001'0000;
		inst |= rs;
		Arm32_BranchAndExchange(cpu, inst);
		break;
	}
	}
}

// Load Address
void Thumb16_LoadAddress(Arm7* cpu, u16 instruction) {
	bool sp = (instruction >> 11) & 1;
	uint rd = (instruction >> 8) & 0b111;
	u32 word8 = instruction & 0xff;

	u32 inst = 0b1110'00'1'0100'0'0000'0000'000000000000; // ADD rd, (), # base
	inst |= (15 - (sp<<1)) << 16;
	inst |= rd << 12;
	inst |= word8;
	Arm32_DataProcessing(cpu, inst);
}

// DEBUG
void Thumb16_DEBUGNOOP(Arm7* cpu, u16 instruction) {

}

// Fetching and Decoding
u16 Thumb16_FetchInstruction(Arm7* cpu) {
	cpu->reg[15] &= 0xffff'fffe;
	if (cpu->canPrint()) std::cout << "\nR15:\t" << std::hex << cpu->reg[15] << std::dec << "\n";
	u16 instruction = cpu->read16(cpu->reg[15]);
	cpu->reg[15] += 2;
	return instruction;
}

typedef void (*ThumbInstructionFunc)(struct Arm7*, u16);
ThumbInstructionFunc Thumb16_Decode(Arm7* cpu, u16 instruction) {
	u32 bits5432109876 = instruction >> 6;

	if ((bits5432109876 & 0b1110000000) == 0b0010000000) {
		std::cout << "Thumb16_MovCmpAddSubImmediate:\t" << std::hex << instruction << std::dec << "\n";
		return &Thumb16_MovCmpAddSubImmediate;
	}
	if ((bits5432109876 & 0b1111110000) == 0b0100010000) {
		std::cout << "Thumb16_HiRegisterOperations:\t" << std::hex << instruction << std::dec << "\n";
		return &Thumb16_HiRegisterOperations;
	}
	if ((bits5432109876 & 0b1111000000) == 0b1010000000) {
		std::cout << "Thumb16_LoadAddress:\t" << std::hex << instruction << std::dec << "\n";
		return &Thumb16_LoadAddress;
	}

	std::cout << "Unimplemented THUMB instruction:\t" << std::hex << instruction << std::dec << "\n";
	cpu->PRINTSTATE();
	return &Thumb16_DEBUGNOOP;
}