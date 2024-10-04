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

	auto comp = [](int a, int b) {
		return a < b;
	};
	std::sort(bgOrder.begin(), bgOrder.end(), comp);

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
		memset(opaquePriority, 0, sizeof opaquePriority);

		int layers = 0;
		for (int i = 0; i < 4; i++)
			layers += (mem->DISPCNT >> (8 + i)) & 1;
		int layersLeft = layers;

		for (int i = 0; i < 4; i++) {
			int bg = bgOrder[i];
			if (!((mem->DISPCNT >> (8 + bg)) & 1))
				continue;

			layersLeft--;

			u16 BGCNT = *mem->BGCNT[bg];
			u32 tileBaseAddr = ((BGCNT >> 2) & 0b11) * 0x4000; // in units of 16 KBytes
			u32 mapBaseAddr = ((BGCNT >> 8) & 0b11111) * 0x800; // in units of 2 KBytes
			bool is8Bpp = (BGCNT & (1 << 7)) != 0;
			u32 bytesPerTile = 32 + 32 * is8Bpp;

			u32 fineY = (vcount + *mem->BGVOFS[bg]) & 7;
			u32 courseY = ((vcount + *mem->BGVOFS[bg]) / 8) & 31;
			for (int x = 0; x < SW; x++) {
				if (opaquePriority[x])
					continue;

				u32 fineX = (x + *mem->BGHOFS[bg]) & 7;
				u32 courseX = ((x + *mem->BGHOFS[bg]) / 8) & 31;

				u32 mapAddr = mapBaseAddr + (courseY * 32 + courseX) * 2;
				//u16 mapData = core->arm7->read16(mapAddr);
				u16 mapData = mem->vram[mapAddr] | (mem->vram[mapAddr + 1] << 8);

				// Flipping. TODO: is it better to just if statement these? check later on
				u32 flipX = 1 * ((mapData >> 10) & 1);
				u32 flipY = 1 * ((mapData >> 11) & 1);

				u32 tileAddr = tileBaseAddr + (mapData & 0x3ff) * bytesPerTile;
				u32 palleteAddr;
				if (is8Bpp) {
					tileAddr += (fineX ^ (7 * flipX));
					tileAddr += (fineY ^ (7 * flipY)) * 8;

					//u32 tileData = core->arm7->read8(tileAddr);
					u32 tileData = mem->vram[tileAddr];
					if (tileData == 0) {
						if (layersLeft > 0)
							continue;
						else
							palleteAddr = 0;
					}
					else
						palleteAddr = tileData * 2;
				}
				else {
					tileAddr += (fineX / 2) ^ (3*flipX);
					tileAddr += (fineY ^ (7*flipY)) * 4;

					//u32 tileData = ((core->arm7->read8(tileAddr) >> ((flipX ^ (fineX & 1)) * 4))) & 0xf;
					u32 tileData = (mem->vram[tileAddr] >> ((flipX ^ (fineX & 1)) * 4)) & 0xf;
					if (tileData == 0) {
						if (layersLeft > 0)
							continue;
						else
							palleteAddr = 0;
					}
					else
						palleteAddr = ((mapData >> 12) & 0xf) * 32 + tileData * 2;
				}

				u32 color = (mem->palleteram[palleteAddr] << 0) | (mem->palleteram[palleteAddr + 1] << 8);
				color = rgb15to24(color);
				opaquePriority[x] = 1;
				//frame[vcount][x] = color;
				framebufferPutPx(x, vcount, color);
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
			//frame[vcount][x] = color;
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
			//frame[vcount][x] = color;
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
		vcountTriggered = false;
		vcount = 0;
		renderFrameToWindow();
	}

	if (vcount == (mem->DISPSTAT >> 8)) {
		vcountTriggered = true;
		if (mem->DISPSTAT & (1 << 5))
			mem->IF |= (1 << 2); // Request Vcounter IRQ
	}
}


void PPU::reset() {
	mem = core->mem;

	vcount = 0;
	scanlineCycles = 0;
	calculateBgOrder();
}