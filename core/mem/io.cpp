int vblank_stub_timer;
int vblank_stub;

#define readLoIO(reg) (reg)
#define readHiIO(reg) ((reg) >> 8)
#define writeLoIO(reg, val) ((reg) = ((reg) & 0xff00) | (val))
#define writeHiIO(reg, val) ((reg) = ((reg) & 0xff) | ((val) << 8))

u8 Mem::read8IO(u32 addr) {
	addr -= 0x0400'0000;
	switch (addr) {

	// DISPCNT
	case 0x0000: return readLoIO(DISPCNT);
	case 0x0001: return readHiIO(DISPCNT);

	// DISPSTAT
	case 0x0004:
		//print("read vblank stub");
		if (vblank_stub_timer++ == 10) {
			vblank_stub_timer = 0;
			vblank_stub ^= 1;
		}
		return vblank_stub;
	case 0x0005:
		return readHiIO(DISPSTAT);

	// VCOUNT
	case 0x0006:
		return core->ppu->vcount;
	case 0x0007:
		return 0;


	// BG*CNT
	case 0x0008: return readLoIO(BG0CNT);
	case 0x0009: return readHiIO(BG0CNT);
	case 0x000a: return readLoIO(BG1CNT);
	case 0x000b: return readHiIO(BG1CNT);
	case 0x000c: return readLoIO(BG2CNT);
	case 0x000d: return readHiIO(BG2CNT);
	case 0x000e: return readLoIO(BG3CNT);
	case 0x000f: return readHiIO(BG3CNT);

	// KEYINPUT
	case 0x0130: return readLoIO(KEYINPUT);
	case 0x0131: return readHiIO(KEYINPUT);

	default: std::cout << "[!] Unimplemented IO Read: " << std::hex << addr + 0x0400'0000 << std::dec << "\n"; return 0;

	}
}

void Mem::write8IO(u32 addr, u32 val) {
	addr -= 0x0400'0000;
	switch (addr) {

	// DISPCNT
	case 0x0000: writeLoIO(DISPCNT, val); break;
	case 0x0001: writeHiIO(DISPCNT, val); break;

	// DISPSTAT
	case 0x0004:
		writeLoIO(DISPSTAT, val & 0b00111000);
		//DISPSTAT = (DISPSTAT & 0xff00) | (val & 0b00111000);
		break;
	case 0x0005:
		//DISPSTAT = (DISPSTAT & 0xff) | (val << 8);
		writeHiIO(DISPSTAT, val);
		break;

	// BG*CNT
	case 0x0008: writeLoIO(BG0CNT, val); break;
	case 0x0009: writeHiIO(BG0CNT, val); break;
	case 0x000a: writeLoIO(BG1CNT, val); break;
	case 0x000b: writeHiIO(BG1CNT, val); break;
	case 0x000c: writeLoIO(BG2CNT, val); break;
	case 0x000d: writeHiIO(BG2CNT, val); break;
	case 0x000e: writeLoIO(BG3CNT, val); break;
	case 0x000f: writeHiIO(BG3CNT, val); break;

	default: std::cout << "[!] Unimplemented IO Write: " << std::hex << addr + 0x0400'0000 << "\t" << val << std::dec << "\n";

	}
}