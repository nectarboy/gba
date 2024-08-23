int vblank_stub_timer;
int vblank_stub;

u8 Mem::read8IO(u32 addr) {
	addr -= 0x0400'0000;
	switch (addr) {

	// DISPCNT
	case 0x0004:
		//print("read vblank stub");
		if (vblank_stub_timer++ == 10) {
			vblank_stub_timer = 0;
			vblank_stub ^= 1;
		}
		return vblank_stub;

	// KEYINPUT
	case 0x0130: return KEYINPUT;
	case 0x0131: return KEYINPUT >> 8;

	default: return 0;

	}
}