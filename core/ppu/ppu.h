#pragma once
struct Core;
struct PPU {
	Core* core;
	PPU(Core* _core) : core(_core) {}

	u32 frame[SH][SW];
	u32 otherBitmapFrame[SH][SW];

	u32 vcount;
	u32 scanlineCycles;

	// Execution
	void drawBackgroundScanline();
	void advanceScanline();
	void renderFrameToWindow();
	void execute(int cpuCycles);

	// Initialization
	void reset();
};