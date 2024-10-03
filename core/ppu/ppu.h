#pragma once

struct Core;
struct Mem;
struct PPU {
	Core* core;
	Mem* mem;
	PPU(Core* _core) : core(_core) {}

	u32 frame[SH][SW];

	u32 vcount;
	u32 scanlineCycles;

	bool vblank;
	bool hblank;
	bool vcountTriggered;

	// Drawing
	u32 opaquePriority[SW];
	std::array<u32, 4> bgOrder{};
	
	void calculateBgOrder();
	void drawBackgroundScanline();

	// Execution
	void advanceScanline();
	void renderFrameToWindow();
	void execute(int cpuCycles);

	// Initialization
	void reset();
};