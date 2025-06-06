#include "constants.cpp"
#include "core/cpu/arm7.h"
#include "core/mem/mem.h"
#include "core/ppu/ppu.h"
#include "core/joypad/joypad.h"

struct Core {
	int test = 69;

	Arm7* arm7;
	Mem* mem;
	Joypad* joypad;
	PPU* ppu;
		
	// Execution
	void execute();
	void executeFrame();
	// Initialization
	void init();
	void reset();
	// Utility
    void loadRomFile(std::string fileName);
	void loadBIOSFile(std::string fileName);
};

void Core::execute() {
	int cpuCycles = arm7->execute();
	ppu->execute(cpuCycles);
}
void Core::executeFrame() {
	joypad->updateKeyStates();
	//std::cout << std::bitset<10>(mem->KEYINPUT) << "\n";

	for (int i = 0; i < 279666; i++) {
		execute();
	}
}

void Core::init() {
	std::cout << "initting core" << "\n";

	arm7 = new Arm7(this);
	std::cout << arm7->test << "\n";

	mem = new Mem(this);

	ppu = new PPU(this);

	joypad = new Joypad(this);

	reset();

}
void Core::reset() {
	mem->reset();
	arm7->reset();
	ppu->reset();
	joypad->reset();
}

void Core::loadRomFile(std::string fileName) {
	std::cout << "\nLoading ROM file:\t\t" << fileName << "... \n";
	std::vector<char> arr = getFileBinaryVector(fileName);
	std::cout << "Size of file:\t\t" << arr.size() << " bytes. \n";

	mem->loadRomArray(arr, arr.size());
	reset();
}
void Core::loadBIOSFile(std::string fileName) {
	std::cout << "\nLoading BIOS file:\t\t" << fileName << "... \n";
	std::vector<char> arr = getFileBinaryVector(fileName);
	std::cout << "Size of file:\t\t" << arr.size() << " bytes. \n";

	mem->loadBIOSArray(arr, arr.size());
}

#include "core/cpu/arm7.cpp"
#include "core/mem/mem.cpp"
#include "core/ppu/ppu.cpp"
#include "core/joypad/joypad.cpp"