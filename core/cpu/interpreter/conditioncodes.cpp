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
bool evalConditionCode(Arm7* cpu, CC cc) {
	switch (cc) {
	case Z_SET: return cpu->cpsr.flagZ;
	case Z_CLR: return !cpu->cpsr.flagZ;
	case C_SET: return cpu->cpsr.flagC;
	case C_CLR: return !cpu->cpsr.flagC;
	case N_SET: return cpu->cpsr.flagN;
	case N_CLR: return !cpu->cpsr.flagN;
	case V_SET: return cpu->cpsr.flagV;
	case V_CLR: return !cpu->cpsr.flagV;
	case C_SET_AND_Z_CLR: return cpu->cpsr.flagC && !cpu->cpsr.flagZ;
	case C_CLR_OR_Z_SET: return !cpu->cpsr.flagC || cpu->cpsr.flagZ;
	case N_EQ_V: return cpu->cpsr.flagN == cpu->cpsr.flagV;
	case N_NEQ_V: return cpu->cpsr.flagN != cpu->cpsr.flagV;
	case Z_CLR_AND_N_EQ_V: return !cpu->cpsr.flagZ && (cpu->cpsr.flagN == cpu->cpsr.flagV);
	case Z_SET_OR_N_NEQ_V: return cpu->cpsr.flagZ || (cpu->cpsr.flagN != cpu->cpsr.flagV);
	case AL: return true;
	case UND: return false;
	default: std::cout << "[!] UNDEFINED CONDITION CODE: " << cc << "\n"; assert(0); return false;
	}
}