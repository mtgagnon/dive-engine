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
    /**
     * @brief Initialises the Vulkan instance and debug messenger.
     * @param window The SDL window to render into.
     */
    VKRenderer(SDL_Window* window);

    /**
     * @brief Destroys the debug messenger and Vulkan instance in the correct order.
     */
    ~VKRenderer();

    /**
     * @brief Begins a new frame.
     * @details Acquires the next swapchain image and begins recording the command buffer.
     */
    void beginFrame();

    /**
     * @brief Ends the current frame.
     * @details Submits the command buffer to the GPU and presents the rendered image to the window.
     */
    void endFrame();

private:

    /**
     * @brief Creates the VkInstance, enabling required SDL and debug extensions.
     * @details Also attaches a temporary debug messenger via pNext to catch errors
     * during instance creation itself.
     */
    void createInstance();

    /**
     * @brief Creates the permanent debug messenger used for the lifetime of the renderer.
     * @details No-op in release builds where validation layers are disabled.
     */
    void setupDebugMessenger();

    /**
     * @brief Checks whether all requested validation layers are available on this machine.
     * @details Validation layers are part of the Vulkan SDK, not the runtime, so they
     * may not be present on machines without the SDK installed.
     * @return true if all layers in the validationLayers list are supported.
     */
    bool checkValidationLayerSupport();

    /**
     * @brief Fills a VkDebugUtilsMessengerCreateInfoEXT with the engine's callback settings.
     * @details Called in two places: once for the temporary pNext messenger during instance
     * creation, and once for the permanent messenger in setupDebugMessenger.
     * @param createInfo The struct to populate.
     */
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);


    // ------- Static Functions -------

    /**
     * @brief Returns the list of Vulkan instance extensions required by SDL and the engine.
     * @details Adds portability extensions on Apple platforms and the debug utils
     * extension when validation layers are enabled.
     * @param window The SDL window, used to query SDL's required extensions.
     * @return List of extension name strings to pass to VkInstanceCreateInfo.
     */
    static std::vector<const char*> getRequiredExtensions(SDL_Window* window);

    /**
     * @brief Vulkan debug callback invoked by the validation layers when an issue is detected.
     * @details Only logs messages at WARNING severity or above. Must use the VKAPI calling
     * convention so Vulkan can call it as a plain C function pointer.
     * @param messageSeverity How severe the message is (verbose, info, warning, error).
     * @param messageType Category of the message (general, validation, performance).
     * @param pCallbackData Contains the human-readable message string and related objects.
     * @param pUserData Optional pointer passed through from messenger creation (unused here).
     * @return VK_FALSE to allow the triggering Vulkan call to continue.
     */
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    /**
     * @brief Loads and calls vkCreateDebugUtilsMessengerEXT at runtime via vkGetInstanceProcAddr.
     * @details This function is not linked directly — it must be looked up through the instance
     * because it is part of an extension rather than core Vulkan.
     * @param instance The Vulkan instance to look up the function from.
     * @param pCreateInfo Messenger configuration (severity, type filters, callback pointer).
     * @param pAllocator Custom allocator, or nullptr to use the default.
     * @param pDebugMessenger Output handle for the created messenger.
     * @return VK_SUCCESS on success, VK_ERROR_EXTENSION_NOT_PRESENT if the extension is unavailable.
     */
    static VkResult createDebugMessenger(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger);

    /**
     * @brief Loads and calls vkDestroyDebugUtilsMessengerEXT at runtime via vkGetInstanceProcAddr.
     * @details Mirror of createDebugMessenger — the destroy function must also be loaded
     * dynamically for the same reason. Does nothing if the function pointer is unavailable.
     * @param instance The Vulkan instance the messenger belongs to.
     * @param debugMessenger The messenger handle to destroy.
     * @param pAllocator Custom allocator, or nullptr to use the default.
     */
    static void destroyDebugMessenger(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks* pAllocator);

    // ------- Variables -------

    VkInstance instance = VK_NULL_HANDLE;  // TODO RAII this as a struct/class member
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;  // TODO RAII this as a struct/class member

    SDL_Window* window = nullptr;
    uint32_t currentFrame = 0;
    uint32_t current_frame_start_timestamp = 0;
};

#endif /* VKRENDERER_H */
