#define SCANLINE_CYCLES 1232
#define HBLANK_CYCLES 272
#define TOTAL_SCANLINES 228

#define	rgb15to24(color) ((((color) >> 0) & 0x1f) << 19) | ((((color) >> 5) & 0x1f) << 11) | ((((color) >> 10) & 0x1f) << 3);

void PPU::renderFrameToWindow() {
	//for (int y = 0; y < SH; y++)
	//	for (int x = 0; x < SW; x++)
	//		framebufferPutPx(x, y, frame[y][x]);
}

// Drawing
void PPU::calculateBgOrder() {
	// Add priority orders on the top two bits
	bgOrder[0] = 0 + ((mem->BG0CNT & 3) << 2); // 0 is highest order, 3 is lowest order in BGCNT
	bgOrder[1] = 1 + ((mem->BG1CNT & 3) << 2);
	bgOrder[2] = 2 + ((mem->BG2CNT & 3) << 2);
	bgOrder[3] = 3 + ((mem->BG3CNT & 3) << 2);

	std::sort(bgOrder.begin(), bgOrder.end());

	// Discard all but the 2 bits after sorting
	bgOrder[0] &= 3;
	bgOrder[1] &= 3;
	bgOrder[2] &= 3;
	bgOrder[3] &= 3;
}

void PPU::drawBackgroundScanline() {
	int bgMode = mem->DISPCNT & 0b111;

	switch (bgMode) {
	case 0: {
		calculateBgOrder(); // TODO: only calculate when bg order changes
		for (int i = 0; i < 4; i++) {
			int bg = bgOrder[i];
			if (!((mem->DISPCNT >> (8 + bg)) & 1))
				continue;

			u16 BGCNT = *mem->BGCNT[bg]; 
			u32 tileBaseAddr = 0x0600'0000 | ((BGCNT >> 2) & 0b11) * 0x4000; // in units of 16 KBytes
			u32 mapBaseAddr = 0x0600'0000 | ((BGCNT >> 8) & 0b11111) * 0x800; // in units of 2 KBytes
			bool is8Bpp = (BGCNT & (1 << 7)) != 0;

			for (int x = 0; x < SW; x++) {
				u32 xx = x / 8;
				u32 yy = vcount / 8;
				u32 mapAddr = mapBaseAddr + (yy * 32 + xx) * 2;
				u16 mapData = core->arm7->read16(mapAddr);

				u32 tileAddr = tileBaseAddr + (mapData & 0x3ff) * (32 + 32*is8Bpp);
				if (is8Bpp) {
					tileAddr += x & 7;
					tileAddr += (vcount & 7) * 8;

					u32 tileData = core->arm7->read8(tileAddr);
					u32 palleteAddr = tileData * 2;

					u32 color = (mem->palleteram[palleteAddr] << 0) | (mem->palleteram[palleteAddr + 1] << 8);
					color = rgb15to24(color);
					frame[vcount][x] = color;
					framebufferPutPx(x, vcount, color);
				}
				else {
					tileAddr += u32((x & 7) / 2);
					tileAddr += ((vcount) & 7) * 4;

					u32 tileData = ((core->arm7->read8(tileAddr) >> ((x & 1) * 4))) & 0xf;
					u32 palleteAddr = ((mapData >> 12) & 0xf) * 32 + tileData * 2;

					u32 color = (mem->palleteram[palleteAddr] << 0) | (mem->palleteram[palleteAddr + 1] << 8);
					color = rgb15to24(color);
					frame[vcount][x] = color;
					framebufferPutPx(x, vcount, color);
				}
			}
		}
		break;
	}
	case 3: {
		u32 baseAddr = vcount * SW;
		for (int x = 0; x < SW; x++) {
			u32 addr = (baseAddr + x) * 2;
			u32 color = (mem->vram[addr] << 0) | (mem->vram[addr + 1] << 8);
			color = rgb15to24(color);
			frame[vcount][x] = color;
			framebufferPutPx(x, vcount, color);
		}
		break;
	}
	case 4: {
		u32 baseAddr = vcount * SW;
		for (int x = 0; x < SW; x++) {
			u32 addr = (baseAddr + x);
			u32 palleteAddr = mem->vram[addr] * 2;
			u32 color = (mem->palleteram[palleteAddr] << 0) | (mem->palleteram[palleteAddr + 1] << 8);
			color = rgb15to24(color);
			frame[vcount][x] = color;
			framebufferPutPx(x, vcount, color);
		}
		break;
	}
	}
}

// Execution
void PPU::execute(int cpuCycles) {
	scanlineCycles += cpuCycles;

	// Visible Scanline finished; HBLANK reached
	if (scanlineCycles >= SCANLINE_CYCLES - HBLANK_CYCLES && !hblank) {
		hblank = true;
		if (mem->DISPSTAT & (1 << 4))
			mem->IF |= (1 << 1); // Request Hblank IRQ
	}
	// Entire Scanline finished
	if (scanlineCycles >= SCANLINE_CYCLES) {
		hblank = false;
		scanlineCycles -= SCANLINE_CYCLES;
		advanceScanline();
	}
}

void PPU::advanceScanline() {
	if (vcount < SH) {
		drawBackgroundScanline();
		vcount++;
		if (vcount == SH) {
			vblank = true;
			if (mem->DISPSTAT & (1 << 3))
				mem->IF |= (1 << 0); // Request Vblank IRQ
		}
	}
	else if (++vcount == 228) {
		vblank = false;
		vcount = 0;
		renderFrameToWindow();
	}
}


void PPU::reset() {
	mem = core->mem;

	vcount = 0;
	scanlineCycles = 0;
	calculateBgOrder();
}