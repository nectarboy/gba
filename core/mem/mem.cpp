struct Core;

struct Mem {
	Core* core;
	Mem(Core* _core) : core(_core) {}

	u8 wramb[0x3ffff];
	u8 wramc[0x7ffff];

	void init();
};

void Mem::init() {

}