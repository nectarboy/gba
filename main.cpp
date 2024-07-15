#include <iostream>
#include <cassert>
#include <bitset>
#include "core/cpu/arm7.h"
//#include "core/cpu/interpreter/arm32.h"
//#include "core/cpu/interpreter/thumb16.h"
#include "math.cpp"

#include "core/cpu/arm7.cpp"
#include "core/cpu/interpreter/arm32.cpp"
#include "core/cpu/interpreter/thumb16.cpp"

int main() {
	Arm7 arm7;
	arm7.init();

	// TESTS
	TEST_MATH();

	std::cout << "setting up";

	return 0;
}