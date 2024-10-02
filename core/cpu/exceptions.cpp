template <Exception exception>
void Arm7::doException() {
	int newMode;
	uint base = 0;
	uint off = 0;
	if constexpr (exception == RESET)					{ newMode = MODE_SVC; base = 0x00; }
	if constexpr (exception == UNDEFINED_INSTRUCTION)	{ newMode = MODE_UND; base = 0x04; }
	if constexpr (exception == SWI)						{ newMode = MODE_SVC; base = 0x08; }
	if constexpr (exception == PREFETCH_ABORT)			{ newMode = MODE_ABT; base = 0x0C; }
	if constexpr (exception == DATA_ABORT)				{ newMode = MODE_ABT; base = 0x10; }
	if constexpr (exception == ADDRESS_EXCEEDS_26BIT)	{ newMode = MODE_SVC; base = 0x14; }
	if constexpr (exception == NORMAL_INTERRUPT)		{ newMode = MODE_IRQ; base = 0x18; off = 4; }
	if constexpr (exception == FAST_INTERRUPT)			{ newMode = MODE_FIQ; base = 0x1C; }

	writeToSPSRModeBank(readCPSR(), newMode);
	setMode(newMode);

	setThumbMode(false);
	cpsr.IRQDisabled = true;
	if constexpr (exception == RESET || exception == FAST_INTERRUPT)
		cpsr.FIQDisabled = true;

	reg[14] = reg[15] + off;
	writeReg(15, base);
}