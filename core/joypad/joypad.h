#pragma once
struct Core;
struct Joypad {
	Core* core;
	Joypad(Core* _core) : core(_core) {}

	bool a;
	bool b;
	bool select;
	bool start;
	bool right;
	bool left;
	bool up;
	bool down;
	bool shoulder_r;
	bool shoulder_l;

	// Debug
	bool pressingP;

	void updateKEYINPUT();
	void updateKeyStates();

	void reset();
};