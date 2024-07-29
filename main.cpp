#include <iostream>
#include <cassert>
#include <bitset>

#include "helpers.h"
#include "math.cpp"

#include "core/core.cpp"

int main() {
	// TESTS
	TEST_MATH();
	
	Core core;
	core.init();

	std::cout << "core has been set up" << "\n";

	return 0;
}