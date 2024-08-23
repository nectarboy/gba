#pragma once
struct Core;
struct Arm7 {
	// Component variables
	Core* core;
	Arm7(Core* _core) : core(_core) {}

	int test = 42;

	// General Registers
	u32 reg[16];
	inline u32 readReg(uint n);
	inline u64 writeReg(uint n, u32 val);
	inline u32 readUserBankReg(uint n);
	inline u32 writeUserBankReg(uint n, u32 val);

	// CPSR (Current Program Status Register)
	struct {
		unsigned int mode : 5;
		bool thumbMode : 1;
		bool FIQDisabled : 1;
		bool IRQDisabled : 1;
		unsigned int : 19; // Reserved (8-27)
		bool flagV : 1;
		bool flagC : 1;
		bool flagZ : 1;
		bool flagN : 1;
	} cpsr;
	u32 readCPSR();
	void writeCPSR(u32 val);

	// Banked Registers
	u32 bankedReg[6][7];
	u32 bankedSpsr[6]; // index 0 is unused
	inline u32 readCurrentSPSR();
	inline void writeCurrentSPSR(u32 val);
	inline void writeToSPSRModeBank(u32 val, uint bankId);

	// Modes and Interrupts
	void setMode(int mode);

	inline void copyCPSRToSPSR();
	inline void copySPSRToCPSR();
	void setThumbMode(bool thumbMode);

	// Reading and Writing to Memory
	u32 openBusVal;
	u32 read8(u32 addr);
	u32 readSigned8(u32 addr);
	u32 read8OptionalSign(u32 addr, bool signextend);
	u32 read16(u32 addr);
	u32 read16OptionalSign(u32 addr, bool signextend);
	u32 readSigned16(u32 addr);
	u32 read32(u32 addr);
	void write8(u32 addr, u8 val);
	void write16(u32 addr, u16 val);
	void write32(u32 addr, u32 val);

	// Execution
	void checkForInterrupts();
	void execute();

	// Initialization
	void bootstrap();
	void reset();

	// Debugging
	u32 _lastPC;
	u64 _executionsRan;
	bool _printEnabled = false;
	void PRINTSTATE();
	void BEFOREFETCH();
	/*constexpr*/ bool canPrint();
};