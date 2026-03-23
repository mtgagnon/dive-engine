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
    VKRenderer(SDL_Window* window);
    ~VKRenderer();

    /*
    Begins a new frame.
    Acquires the swapchain image, begins the command buffer, etc.
    */
    void beginFrame();

    /*
    Ends the frame.
    Submits the command buffer, presents the image, etc.
    */
    void endFrame();

private:

    /*
    Creates the Vulkan instance.
    */
    void createInstance();

    void setupDebugMessenger();

    bool checkValidationLayerSupport();

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

    SDL_Window* window = nullptr;
    uint32_t currentFrame = 0;
    uint32_t current_frame_start_timestamp = 0;
};

#endif /* VKRENDERER_H */
