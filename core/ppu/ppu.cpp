#define SCANLINE_CYCLES 1232
#define HBLANK_CYCLES 272
#define TOTAL_SCANLINES 228

#define	rgb15to24(color) ((((color) >> 0) & 0x1f) << 19) | ((((color) >> 5) & 0x1f) << 11) | ((((color) >> 10) & 0x1f) << 3);

void PPU::renderFrameToWindow() {
	for (int y = 0; y < SH; y++)
		for (int x = 0; x < SW; x++)
			framebufferPutPx(x, y, frame[y][x]);
}

void PPU::execute(int cpuCycles) {
	scanlineCycles += cpuCycles;

	// Visible Scanline finished; HBLANK reached
	if (scanlineCycles >= SCANLINE_CYCLES - HBLANK_CYCLES) {
		// Interrupt or something
	}
	// Entire Scanline finished
	if (scanlineCycles >= SCANLINE_CYCLES) {
		scanlineCycles -= SCANLINE_CYCLES;
		advanceScanline();
	}
}

void PPU::drawBackgroundScanline() {
	int bgMode = core->mem->DISPCNT & 0b111;

	if (bgMode == 3) {
		for (int x = 0; x < SW; x++) {
			u32 addr = (vcount * SW + x) * 2;
			u32 color = (core->mem->vram[addr] << 0) | (core->mem->vram[addr + 1] << 8);
			frame[vcount][x] = rgb15to24(color);
		}
	}
	else if (bgMode == 4) {
		for (int x = 0; x < SW; x++) {
			u32 addr = (vcount * SW + x);
			u32 palleteAddr = core->mem->vram[addr] * 2;
			u32 color = (core->mem->palleteram[palleteAddr] << 0) | (core->mem->palleteram[palleteAddr + 1] << 8);
			frame[vcount][x] = rgb15to24(color);
		}
	}

}

void PPU::advanceScanline() {
	drawBackgroundScanline();

	if (++vcount == 228) {
		vcount = 0;
		renderFrameToWindow();
	}
}


void PPU::reset() {
	vcount = 0;
	scanlineCycles = 0;
}