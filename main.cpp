#include <stdio.h>
#include <stdlib.h>
#include <cassert>

#include <iostream>
#include <fstream>
#include <vector>
#include <bitset>

#include "helpers.h"
#include "math.cpp"

#include "core/core.cpp"

int main() {
	// TESTS
	TEST_MATH();
	TEST_ARM32DECODE();
	
	Core core;
	core.init();
	core.loadRomFile("./roms/panda.gba");

	std::cout << "\ncore has been set up" << "\n";

	for (int i = 0; i < 10'000'000; i++)
		core.execute();

	return 0;
}