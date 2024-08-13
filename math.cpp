inline u64 bitRotateLeft(u64 val, uint n, uint shift) {
	//if (shift == n) // Safety
	//	return val;
	return (val << shift) | (val >> (n - shift)) /*& ((1 << n) - 1)*/;
}
inline u64 bitRotateRight(u64 val, uint n, uint shift) {
	//if (shift == n) // Safety
	//	return val;
	return (val >> shift) | (val << (n - shift)) /*& ((1 << n) - 1)*/;
}
inline u64 bitShiftLeft(u64 val, uint n, uint shift) {
	//if (shift >= n) // Safety for
	//	return 0;
	return (val << shift) /*& ((1 << n) - 1)*/;
}
inline u64 bitShiftRight(u64 val, uint n, uint shift) {
	//if (shift >= n) // Safety
	//	return 0;
	return (val >> shift) /*& ((1 << n) - 1)*/;
}
inline u64 bitSignedShiftRight(u64 val, uint n, uint shift) {
	// On s64:
	//return (u64)(((s32)(val)) >> shift);
	// On u64:
	//return (val >> shift) | ((val >> (n - 1))
	//	* (((1ULL << n) - 1) & ~((1ULL << (n - shift)) - 1)));

	// Credits to BreadFish64 for making the above clusterfuck readable:
	const u64 signbit = (val >> (n - 1));
	// Safety :(
	if (n == shift) {
		if (signbit)
			return (1ULL << n) - 1;
		else
			return 0;
	}

	const u64 activemask = (1ULL << n) - 1;
	const u64 keepmask = (1ULL << (n - shift)) - 1;
	const u64 fillmask = activemask & ~keepmask;
	return (val >> shift) | (signbit * fillmask);
}

// TESTS
void TEST_MATH() {
	// shift right
	assert(bitShiftRight(0x8000'0000, 32, 31) == 1);
	assert(bitShiftRight(0x8000'0000, 32, 32) == 0);

	// rotate right
	assert(bitRotateRight(0x96, 32, 24) == 0x9600);

	// signed right shift
	assert(bitSignedShiftRight(0x8000'0000, 32, 4) == 0xf800'0000);
	assert(bitSignedShiftRight(0x8000'0000, 32, 32) == 0xffff'ffff);
	assert(bitSignedShiftRight(0x0000'0100, 32, 4) == 0x0000'0010);
	assert(bitSignedShiftRight(0x0000'0100, 32, 32) == 0);

	assert(0x7fff'ffff + u32(-(s32)(0x7fff'ffff)) == 0);
}