struct Core;

struct Mem {
	Core* core;
	Mem(Core* _core) : core(_core) {}

	u8 wramb[0x3ffff];
	u8 wramc[0x7ffff];

	u32 romSize;
	u8* rom = new u8[0];

	void init();
	void loadRomArray(std::vector<char>& arr, u64 size);
};

void Mem::init() {
	for (int i = 0; i < lenOfArray(wramb); i++)
		wramb[i] = 0;
	for (int i = 0; i < lenOfArray(wramc); i++)
		wramc[i] = 0;
}

void Mem::loadRomArray(std::vector<char>& arr, u64 size) {
	romSize = 0;
	if (size > 0x1ffffff) {
		size = 0x2000000; // Max 32MB
		romSize = size;
	}
	else {
		while (romSize < size) {
			romSize <<= 1;
			romSize |= 1;
		}
		romSize += 1;
	}
	std::cout << "Size rounded up:\t" << romSize << " bytes (" << romSize / 1024 << " kb)\n";

	delete[] rom;
	rom = new u8[romSize];
	for (u32 i = 0; i < size; i++)
		rom[i] = u8(arr[i]);
}