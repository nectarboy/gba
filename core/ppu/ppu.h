#pragma once
struct Core;
struct Mem;
struct PPU {
	Core* core;
	Mem* mem;
	PPU(Core* _core) : core(_core) {}

	u32 frame[SH][SW];
	u32 otherBitmapFrame[SH][SW];

	u32 vcount;
	u32 scanlineCycles;

	bool vblank;
	bool hblank;
	bool vcountTriggered;

	// Execution
	void drawBackgroundScanline();
	void advanceScanline();
	void renderFrameToWindow();
	void execute(int cpuCycles);

	// Initialization
	void reset();
};