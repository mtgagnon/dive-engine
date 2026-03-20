//
//  vulkan_test_main.cpp
//  Standalone test for VulkanRenderer - renders a spinning triangle
//
//  To build and run:
//  1. Compile shaders:
//     glslc resources/shaders/triangle.vert -o resources/shaders/triangle.vert.spv
//     glslc resources/shaders/triangle.frag -o resources/shaders/triangle.frag.spv
//
//  2. Build with CMake (cmake will pick this up via GLOB)
//
//  3. Run: ./dive_engine
//

#include "VulkanRenderer.h"
#include "SDL2/SDL.h"
#include "SDL_vulkan.h"

#include <iostream>
#include <stdexcept>

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create window with Vulkan flag
    SDL_Window* window = SDL_CreateWindow(
        "Vulkan Test - Spinning Triangle",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        800,
        600,
        SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Create and initialize Vulkan renderer
    VulkanRenderer renderer;

    try {
        renderer.initialize(window);
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize Vulkan: " << e.what() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Main loop
    bool running = true;
    SDL_Event event;

    std::cout << "Rendering spinning triangle. Press ESC or close window to exit." << std::endl;

    while (running) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                }
            }
        }

        // Render frame
        try {
            renderer.beginFrame();
            renderer.endFrame();
        } catch (const std::exception& e) {
            std::cerr << "Render error: " << e.what() << std::endl;
            running = false;
        }
    }

    // Cleanup
    renderer.cleanup();
    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << "Clean shutdown." << std::endl;
    return 0;
}
