#include "core/cpu/arm7.h"
#include "core/mem/mem.h"

std::vector<char> getFileBinaryVector(std::string fileName) {
	// So much cleaner than C
	std::ifstream file(fileName, std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		printAndCrash("File not found... Exiting.");
	}
	u64 size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> bin(size);
	file.read(bin.data(), size);

	return bin;
}

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
	void loadBIOSFile(std::string fileName);
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
	std::cout << "\nLoading ROM file:\t\t" << fileName << "... \n";
	std::vector<char> arr = getFileBinaryVector(fileName);
	std::cout << "Size of file:\t\t" << arr.size() << " bytes. \n";

	mem->loadRomArray(arr, arr.size());
	this->reset();
}
void Core::loadBIOSFile(std::string fileName) {
	std::cout << "\nLoading BIOS file:\t\t" << fileName << "... \n";
	std::vector<char> arr = getFileBinaryVector(fileName);
	std::cout << "Size of file:\t\t" << arr.size() << " bytes. \n";

	mem->loadBIOSArray(arr, arr.size());
}

#include "core/cpu/arm7.cpp"
#include "core/mem/mem.cpp"