#include "core/cpu/arm7.cpp"
#include "core/mem/mem.cpp"

struct Core {
	int test = 69;

	Arm7* arm7;
	Mem* mem;

	void init();
};

void Core::init() {
	std::cout << "initting core" << "\n";

	arm7 = new Arm7(this);
	std::cout << arm7->test << "\n";
	arm7->init();

	mem = new Mem(this);
	mem->init();
}