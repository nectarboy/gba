#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <cassert>

#include <thread>
#include <chrono>

#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <bitset>

#include "helpers.h"
#include "math.cpp"

int globaltest = 123;
std::map<int, bool> keyboard;

#include "constants.cpp"
#include "framebuffer.cpp"
#include "core/core.cpp"

int main(int argc, char* argv[]) {
	// TESTS
	TEST_MATH();
	TEST_ARM32DECODE();
	
	Core core;
	core.init();
    core.loadBIOSFile(BIOSPATH);
	core.loadRomFile(TESTROMPATH);

	std::cout << "\ncore has been set up" << "\n";

    // SETUP SDL SHIT
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL ERROR :(");
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow
    (
        EMU_NAME,
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
    SDL_Event sdlEvent;

    // Program loop ...
    bool running = true;
    while (running)
    {
        // -------------------- //
        // Wait / Check for events
        while (SDL_PollEvent(&sdlEvent))
        {
            switch (sdlEvent.type)
            {
            // General
            case SDL_QUIT:
                running = false;
                break;

            // Input
            case SDL_KEYDOWN:
                keyboard[sdlEvent.key.keysym.sym] = true;
                //std::cout << "yea " << std::hex << sdlEvent.key.keysym.sym << " " << SDLK_UP << std::dec << "\n";
                break;
            case SDL_KEYUP:
                keyboard[sdlEvent.key.keysym.sym] = false;
                //std::cout << "no \n";
                break;
            }
        }
        // -------------------- //

        core.executeFrame();

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