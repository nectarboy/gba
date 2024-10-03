#pragma once
struct Core;
struct Mem {
	Core* core;
	Mem(Core* _core) : core(_core) {}

	int test = 420;

	// Memory Regions
	u8 bios[0x4000];
	u8 wramb[0x40000];
	u8 wramc[0x8000];
	u8 palleteram[0x400];
	u8 vram[0x18000];

	u32 romSize;
	u8* rom = new u8[0];

	// IO Registers
	// TODO: make sure to reset all these by writing 0s to all of IO on reset
	u16 DISPCNT;
	u16 DISPSTAT;
	u16 BG0CNT;
	u16 BG1CNT;
	u16 BG2CNT;
	u16 BG3CNT;
	u16 BG0HOFS;
	u16 BG0VOFS;
	u16 BG1HOFS;
	u16 BG1VOFS;
	u16 BG2HOFS;
	u16 BG2VOFS;
	u16 BG3HOFS;
	u16 BG3VOFS;
	u16* BGCNT[4] = { &BG0CNT, &BG1CNT, &BG2CNT, &BG3CNT };
	u16* BGHOFS[4] = { &BG0HOFS, &BG1HOFS, &BG2HOFS, &BG3HOFS };
	u16* BGVOFS[4] = { &BG0VOFS, &BG1VOFS, &BG2VOFS, &BG3VOFS };

	u16 KEYINPUT;

	bool IME;
	u16 IE;
	u16 IF;

	// Methods
	u8 read8IO(u32 addr);
	void write8IO(u32 addr, u32 val);

	void reset();
	void loadRomArray(std::vector<char>& arr, u64 size);
	void loadBIOSArray(std::vector<char>& arr, u64 size);
};