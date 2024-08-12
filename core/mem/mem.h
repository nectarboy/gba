#pragma once
struct Core;
struct Mem {
	Core* core;
	Mem(Core* _core) : core(_core) {}

	int test = 420;

	u8 wramb[0x40000];
	u8 wramc[0x8000];
	u8 palleteram[0x400];
	u8 vram[0x18000];

	// IO
	u16 dspcnt;

	u32 romSize;
	u8* rom = new u8[0];

	void reset();
	void loadRomArray(std::vector<char>& arr, u64 size);
};