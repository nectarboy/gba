#pragma once
struct Core;
struct Mem {
	Core* core;
	Mem(Core* _core) : core(_core) {}

	int test = 420;

	u8 wramb[0x3ffff];
	u8 wramc[0x7ffff];

	u32 romSize;
	u8* rom = new u8[0];

	void reset();
	void loadRomArray(std::vector<char>& arr, u64 size);
};