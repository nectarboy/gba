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
	
	Core core;
	core.init();
	core.loadRomFile("./roms/panda.gba");

	std::cout << "\ncore has been set up" << "\n";

	return 0;
}