#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <cassert>

#include <thread>
#include <chrono>

#include <iostream>
#include <fstream>
#include <vector>
#include <bitset>

#include "helpers.h"
#include "math.cpp"

#include "constants.cpp"
#include "framebuffer.cpp"
#include "core/core.cpp"

int main(int argc, char* argv[]) {
	// TESTS
	TEST_MATH();
	TEST_ARM32DECODE();
	
	Core core;
	core.init();
	core.loadRomFile("./roms/panda.gba");

	std::cout << "\ncore has been set up" << "\n";

    // SETUP SDL SHIT
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL ERROR :(");
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow
    (
        "Gerber Baby Advance",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCALE * SW, SCALE * SH,
        SDL_WINDOW_SHOWN
    );
    SDL_Renderer* windowRenderer = SDL_CreateRenderer
    (
        window,
        0,
        SDL_RENDERER_ACCELERATED
    );
    SDL_Texture* texture = SDL_CreateTexture
    (
        windowRenderer,
        SDL_PIXELFORMAT_ARGB32,
        SDL_TEXTUREACCESS_STREAMING,
        SCALE * SW, SCALE * SH
    );
    SDL_Event windowEvent;

    // Program loop ...
    bool running = true;
    while (running)
    {
        // -------------------- //
        // Wait / Check for events
        while (SDL_PollEvent(&windowEvent))
        {
            switch (windowEvent.type)
            {
            case SDL_QUIT:
                running = false;
                break;
            }
        }
        // -------------------- //

        // Run emulator
        for (int i = 0; i < 279666; i++)
            core.execute();

        // crappy draw
        for (int x = 0; x < SW; x++) {
            for (int y = 0; y < SH; y++) {
                u32 color = core.arm7->read16(0x0600'0000 + (y * SW + x) * 2);
                //std::cout << color << "\n";
                framebufferPutPx(x, y, color);
            }
        }

        SDL_UpdateTexture(texture, NULL, frameBuffer, BUFFER_BYTES_PER_ROW);

        // Render
        SDL_RenderClear(windowRenderer);
        SDL_RenderCopy(windowRenderer, texture, NULL, NULL);
        SDL_RenderPresent(windowRenderer);

        std::this_thread::sleep_for(std::chrono::nanoseconds(1'000'000 / 60));
    }

    // When quit- destroy SDL
    SDL_DestroyRenderer(windowRenderer);
    SDL_DestroyWindow(window);
    SDL_DestroyTexture(texture);
    SDL_Quit();

	return 0;
}