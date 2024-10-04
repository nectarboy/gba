void Joypad::updateKEYINPUT() {
	core->mem->KEYINPUT = (!a) | (!b << 1) | (!select << 2) | (!start << 3) | (!right << 4) | (!left << 5) | (!up << 6) | (!down << 7) | (!shoulder_r << 8) | (!shoulder_l << 9);
}

void Joypad::updateKeyStates() {
	up =			keyboard[SDLK_UP];
	down =			keyboard[SDLK_DOWN];
	left =			keyboard[SDLK_LEFT];
	right =			keyboard[SDLK_RIGHT];
	a =				keyboard[SDLK_z];
	b =				true;//keyboard[SDLK_x]; // Temporary
	shoulder_r =	keyboard[SDLK_s];
	shoulder_l =	keyboard[SDLK_a];
	start =			keyboard[SDLK_RETURN];
	select =		keyboard[SDLK_RSHIFT];

	updateKEYINPUT();
}

void Joypad::reset() {
	updateKEYINPUT();
}