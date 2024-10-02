void Joypad::updateKEYINPUT() {
	core->mem->KEYINPUT = (!a) | (!b << 1) | (!select << 2) | (!start << 3) | (!right << 4) | (!left << 5) | (!up << 6) | (!down << 7) | (!shoulder_r << 8) | (!shoulder_l << 9);
}

void Joypad::updateKeyStates() {
	up =			keyboard[SDLK_UP];
	down =			keyboard[SDLK_DOWN];
	left =			keyboard[SDLK_LEFT];
	right =			keyboard[SDLK_RIGHT];
	a =				keyboard[SDLK_z];
	b =				keyboard[SDLK_x];
	shoulder_r =	keyboard[SDLK_a];
	shoulder_l =	keyboard[SDLK_s];
	start =			keyboard[SDLK_RETURN];
	select =		keyboard[SDLK_RSHIFT];

	if (keyboard[SDLK_p])
		core->arm7->_printEnabled = true;
	else
		core->arm7->_printEnabled = false;

	updateKEYINPUT();
}

void Joypad::reset() {
	updateKEYINPUT();
}