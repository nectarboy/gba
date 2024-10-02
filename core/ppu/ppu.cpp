#define SCANLINE_CYCLES 1232
#define HBLANK_CYCLES 272
#define TOTAL_SCANLINES 228

#define	rgb15to24(color) ((((color) >> 0) & 0x1f) << 19) | ((((color) >> 5) & 0x1f) << 11) | ((((color) >> 10) & 0x1f) << 3);

void PPU::renderFrameToWindow() {
	//for (int y = 0; y < SH; y++)
	//	for (int x = 0; x < SW; x++)
	//		framebufferPutPx(x, y, frame[y][x]);
}

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

void PPU::drawBackgroundScanline() {
	int bgMode = mem->DISPCNT & 0b111;

	if (bgMode == 3) {
		u32 baseAddr = vcount * SW;
		for (int x = 0; x < SW; x++) {
			u32 addr = (baseAddr + x) * 2;
			u32 color = (mem->vram[addr] << 0) | (mem->vram[addr + 1] << 8);
			color = rgb15to24(color);
			frame[vcount][x] = color;
			framebufferPutPx(x, vcount, color);
		}
	}
	else if (bgMode == 4) {
		u32 baseAddr = vcount * SW;
		for (int x = 0; x < SW; x++) {
			u32 addr = (baseAddr + x);
			u32 palleteAddr = mem->vram[addr] * 2;
			u32 color = (mem->palleteram[palleteAddr] << 0) | (mem->palleteram[palleteAddr + 1] << 8);
			color = rgb15to24(color);
			frame[vcount][x] = color;
			framebufferPutPx(x, vcount, color);
		}
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
}