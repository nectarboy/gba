void Mem::reset() {
	for (int i = 0; i < lenOfArray(wramb); i++)
		wramb[i] = 0;
	for (int i = 0; i < lenOfArray(wramc); i++)
		wramc[i] = 0;
	for (int i = 0; i < lenOfArray(vram); i++)
		palleteram[i] = 0;
	for (int i = 0; i < lenOfArray(vram); i++)
		vram[i] = 0;
}

void Mem::loadRomArray(std::vector<char>& arr, u64 size) {
	romSize = 0;
	if (size > 0x1ffffff) {
		size = 0x2000000; // Max 32MB
		romSize = size;
	}
	else {
		while (romSize < size) {
			romSize <<= 1;
			romSize |= 1;
		}
		romSize += 1;
	}
	std::cout << "Size rounded up:\t" << romSize << " bytes (" << romSize / 1024 << " kb)\n";

	delete[] rom;
	rom = new u8[romSize];
	for (u32 i = 0; i < size; i++)
		rom[i] = u8(arr.data()[i]);
}

void Mem::loadBIOSArray(std::vector<char>& arr, u64 size) {
	if (size != 0x4000)
		std::cout << "WARNING: file size not equal to 16KB, the BIOS is zero filled/capped at 16KB.\n";

	if (size > 0x4000)
		size = 0x4000;

	for (int i = 0; i < 0x4000; i++) // Zero fill first
		bios[i] = 0;
	for (int i = 0; i < size; i++)
		bios[i] = u8(arr.data()[i]); // Load data next
}