//
//  VKRenderer.h
//  dive_engine
//

#ifndef VKRENDERER_H
#define VKRENDERER_H

#include <string>
#include <vector>
#include <optional>

#include <vulkan/vulkan.h>
#include "SDL2/SDL.h"


class VKRenderer {
public:
    void initialize(SDL_Window* window);
    void cleanup();

    void beginFrame();
    void endFrame();

    bool isInitialized() const { return initialized; };

private:

    SDL_Window* window = nullptr;
    uint32_t currentFrame = 0;
    uint32_t current_frame_start_timestamp = 0;
    bool initialized = false;
};

#endif /* VKRENDERER_H */
