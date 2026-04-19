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

    // ------- Inner Types / Structs -------
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() const {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    /**
     * @brief Swapchain support details struct.
     * @struct SwapchainSupportDetails
     * @field capabilities The surface capabilities.
     * @field formats The available surface formats.
     * @field presentModes The available present modes.
     */
    struct SwapchainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats = {};
        std::vector<VkPresentModeKHR> presentModes = {};
    };

    // ------- Functions -------

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
     * @brief Loads and calls vkCreateDebugUtilsMessengerEXT at runtime via vkGetInstanceProcAddr.
     * @details This function is not linked directly — it must be looked up through the instance
     * because it is part of an extension rather than core Vulkan.
     * @param pCreateInfo Messenger configuration (severity, type filters, callback pointer).
     * @return VK_SUCCESS on success, VK_ERROR_EXTENSION_NOT_PRESENT if the extension is unavailable.
     */
     VkResult createDebugMessenger(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo);

     /**
      * @brief Loads and calls vkDestroyDebugUtilsMessengerEXT at runtime via vkGetInstanceProcAddr.
      * @details Mirror of createDebugMessenger — the destroy function must also be loaded
      * dynamically for the same reason. Does nothing if the function pointer is unavailable.
      */
     void destroyDebugMessenger();

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

    /**
     * @brief Picks the first suitable physical device to use.
     TODO: Add more sophisticated picking logic. Possibly support multiple GPUs.
     */
    void pickPhysicalDevice();

    /**
     * @brief Checks if the device is suitable.
     * @param device The device to check.
     * @return True if the device is suitable.
     */
    bool isDeviceSuitable(VkPhysicalDevice device);

    /**
     * @brief Finds the queue families for the device.
     * @param device The device to find the queue families for.
     * @return The queue families.
     */
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    /**
     * @brief Creates the logical device.
     * @details Creates the logical device and queues for the physical device.
     */
    void createLogicalDevice();

    /**
     * @brief Destroys the logical device.
     * @details Destroys the logical device and queues for the physical device.
     */
    void destroyLogicalDevice();

    /**
     * @brief Creates the surface.
     * @details Creates the surface for the window.
     */
    void createSurface();

    /**
     * @brief Destroys the surface.
     * @details Destroys the surface for the window.
     */
    void destroySurface();

    /**
     * @brief Queries the swapchain support details.
     * @details Queries the swapchain support details for the physical device.
     * @param device The physical device to query the swapchain support details for.
     * @return The swapchain support details.
     */
    SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device);

    /**
     * @brief Chooses the swapchain surface format.
     * @details Chooses the swapchain surface format for the physical device.
     * @param formats The available formats.
     * @return The chosen format.
     */
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);

    /**
     * @brief Chooses the swapchain present mode.
     * @details Chooses the swapchain present mode for the physical device.
     * @param modes The available present modes.
     * @return The chosen present mode.
     */
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& modes);

    /**
     * @brief Chooses the swapchain extent.
     * @details Chooses the swapchain extent for the physical device.
     * @param capabilities The available capabilities.
     * @return The chosen extent.
     */
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    /**
     * @brief Creates the swapchain.
     * @details Creates the swapchain for the physical device.
     */
    void createSwapchain();

    /**
     * @brief Destroys the swapchain.
     * @details Destroys the swapchain for the physical device.
     */
    void destroySwapchain();

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

    // ------- Variables -------

    // TODO RAII these as a struct/class member
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkDevice logicalDevice = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;

    // swapchain
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages;
    VkFormat swapchainImageFormat;


VkExtent2D swapchainExtent;

    SDL_Window* window = nullptr;
    uint32_t currentFrame = 0;
    uint32_t current_frame_start_timestamp = 0;
};

#endif /* VKRENDERER_H */
