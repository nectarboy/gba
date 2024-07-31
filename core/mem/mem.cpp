struct Core;

struct Mem {
	Core* core;
	Mem(Core* _core) : core(_core) {}

	u8 wramb[0x3ffff];
	u8 wramc[0x7ffff];

	bool romPresent;
	u8 rom[];

	void init();
	void loadRom();
};

void Mem::init() {
	for (int i = 0; i < lenOfArray(wramb); i++) wramb[i] = 0;
	for (int i = 0; i < lenOfArray(wramc); i++) wramc[i] = 0;
}

void Mem::loadRom(*arr) {
	delete[] rom;


}