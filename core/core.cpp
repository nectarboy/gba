#include "core/cpu/arm7.h"
#include "core/mem/mem.h"

struct Core {
	int test = 69;

	Arm7* arm7;
	Mem* mem;
		
	// Execution
	void execute();
	// Initialization
	void init();
	void reset();
	// Utility
    void loadRomFile(std::string fileName);
};

void Core::execute() {
	arm7->execute();
}

void Core::init() {
	std::cout << "initting core" << "\n";

	arm7 = new Arm7(this);
	std::cout << arm7->test << "\n";

	mem = new Mem(this);
}
void Core::reset() {
	arm7->reset();
	mem->reset();
}

void Core::loadRomFile(std::string fileName) {
	std::cout << "\nLoading file:\t\t" << fileName << "... \n";

	// So much cleaner than C
	std::ifstream file(fileName, std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		printAndCrash("File not found... Exiting.");
	}
	u64 size = file.tellg();
	std::cout << "Size of file:\t\t" << size << " bytes. \n";
	file.seekg(0, std::ios::beg);

	std::vector<char> arr(size);
	file.read(arr.data(), size);

	mem->loadRomArray(arr, size);
	this->reset();
}

#include "core/cpu/arm7.cpp"
#include "core/mem/mem.cpp"