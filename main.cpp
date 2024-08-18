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
        "Banana Boy, Advance!",
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
                if (FAUXMODE3) {
                    u32 addr = (y * SW + x) * 2;
                    u32 color = (core.mem->vram[addr] << 0) | (core.mem->vram[addr + 1] << 8);
                    color = (((color >> 0) & 0x1f) << 19) | (((color >> 5) & 0x1f) << 11) | (((color >> 10) & 0x1f) << 3);
                    framebufferPutPx(x, y, color);
                }
                else {
                    u32 addr = (y * SW + x);
                    u32 palleteaddr = core.mem->vram[addr]*2;
                    u32 color = (core.mem->palleteram[palleteaddr] << 0) | (core.mem->palleteram[palleteaddr + 1] << 8);
                    color = (((color >> 0) & 0x1f) << 19) | (((color >> 5) & 0x1f) << 11) | (((color >> 10) & 0x1f) << 3);
                    //color = color ? 0xffffff : 0;
                    framebufferPutPx(x, y, color);
                }
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