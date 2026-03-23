//
//  main.cpp
//  dive_engine
//
//  Created by Mathurin Gagnon on 1/24/24.
//

#define SDL_MAIN_HANDLED

#include <stdio.h>
#include <iostream>

#include "Engine.h"
// #include "OGLRenderer.h"
#include <filesystem>
#include "VKRenderer.h"
using std::cout, std::cin, std::string;

// inline static SDL_Renderer* renderer = nullptr;

SDL_Window* createWindow(const std::string& game_title, int x_resolution, int y_resolution) {
    if(SDL_Init(SDL_INIT_VIDEO) != 0) {
        cout << "SDL could not initialize! SDL_ERROR: " << SDL_GetError() << std::endl;
        exit(0);
    }

    SDL_Window* window = SDL_CreateWindow(
        game_title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        x_resolution,
        y_resolution,
        SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN  // Change to SDL_WINDOW_VULKAN when using VKRenderer
    );

    if(!window) {
        cout << "Window could not be created! SDL Error: " << SDL_GetError() << std::endl;
        exit(0);
    }

    return window;
}

int main(int argc, char* argv[]){

    // Engine engine;
    // engine.game_loop();

    // testing and sandboxing with VKRenderer

    SDL_Window* window = createWindow("Vulkan Test", 2*640, 2*360);
    VKRenderer renderer(window);
    bool running = true;

    try {
        while (running) {
            renderer.beginFrame();
            // calls to the renderer happen here
            renderer.endFrame();
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                } else if (event.type == SDL_KEYDOWN) {
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        running = false;
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
