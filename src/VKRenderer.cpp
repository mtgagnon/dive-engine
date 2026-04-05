//
//  VKRenderer.cpp
//  dive_engine
//

#include "VKRenderer.h"
#include "SDL_vulkan.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "glm/gtc/matrix_transform.hpp"

#include <iostream>
#include <stdexcept>
#include <set>
#include <algorithm>
#include <limits>
#include <cstring>

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};
// constructor
VKRenderer::VKRenderer(SDL_Window* window) {
    // order of creation is as follows:
    // 1. window
    // 2. instance
    // 4. debug messenger
    // 5. create surface
    // 6. pick physical device
    // 7. logical device

    this->window = window;

    createInstance();
    setupDebugMessenger();  // will only do things if validation layers are enabled
    createSurface(); // must be before pickPhysicalDevice
    pickPhysicalDevice();     // needs surface to check present queue support
    createLogicalDevice();    // needs physicalDevice from above
}

// destructor
VKRenderer::~VKRenderer() {
    // order for destruction in reverse order of creation order is as follows:
    // 1. logical device
    // 2. surface
    // 3. debug messenger
    // 4. instance

    destroyLogicalDevice();
    destroySurface();
    destroyDebugMessenger(); // will only do things if validation layers are enabled
    vkDestroyInstance(this->instance, nullptr);
}

// begin frame3
void VKRenderer::beginFrame() {
    // std::cout << "VKRenderer beginFrame!" << std::endl;
}

// end frame
void VKRenderer::endFrame() {
    // std::cout << "VKRenderer endFrame!" << std::endl;
}

void VKRenderer::createInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("Validation layers requested but not available!");
    }

    #if !defined(NDEBUG)
    std::cout << "Validation layers enabled!" << std::endl;
    #endif

    // application info
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Dive Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Dive Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    // get extensions from SDL
    std::vector<const char*> extensions = getRequiredExtensions(this->window);

    // instance creation info
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    #ifdef __APPLE__
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    #endif

    // need to add validation layers if they are enabled to instance creation info
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{}; // must be oustide so it can be used in the pNext chain
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    // create and validate the instance
    if (vkCreateInstance(&createInfo, nullptr, &this->instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance");
    }

    #if !defined(NDEBUG)
    std::cout << "Vulkan instance created successfully" << std::endl;
    #endif
}

std::vector<const char*> VKRenderer::getRequiredExtensions(SDL_Window* window) {
    unsigned int sdlExtensionCount = 0;
    SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, nullptr);
    std::vector<const char*> extensions(sdlExtensionCount);
    SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, extensions.data());

    #ifdef __APPLE__
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    extensions.push_back("VK_KHR_get_physical_device_properties2");
    #endif

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

bool VKRenderer::checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }
    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VKRenderer::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
    }
    return VK_FALSE;
}

void VKRenderer::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;
}

// create debug messenger
VkResult VKRenderer::createDebugMessenger(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(this->instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(this->instance, pCreateInfo, nullptr, &this->debugMessenger);
    }
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

// destroy debug messenger
void VKRenderer::destroyDebugMessenger() {
    if (!enableValidationLayers) return;
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(this->instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(this->instance, this->debugMessenger, nullptr);
    }

    #if !defined(NDEBUG)
    std::cout << "Debug messenger destroyed successfully" << std::endl;
    #endif
}

// setup debug messenger
void VKRenderer::setupDebugMessenger() {
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (createDebugMessenger(&createInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to set up debug messenger");
    }
}

void VKRenderer::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(this->instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(this->instance, &deviceCount, devices.data());


    for(const auto& pdevice : devices) {
        if(isDeviceSuitable(pdevice)) {
            this->physicalDevice = pdevice;
            break; // pick the first suitable device
        }
    }

    if(this->physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable GPU");
    }

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(this->physicalDevice, &props);

    #if !defined(NDEBUG)
    std::cout << "Selected GPU: " << props.deviceName << " with " << props.deviceType << " type" << std::endl;
    #endif
}

bool VKRenderer::isDeviceSuitable(VkPhysicalDevice device) {
    return findQueueFamilies(device).isComplete();
}

VKRenderer::QueueFamilyIndices VKRenderer::findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for(const auto& queueFamily : queueFamilies) {
        if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, this->surface, &presentSupport);
        if(presentSupport) {
            indices.presentFamily = i;
        }

        if(indices.isComplete()) {
            break;
        }
        i++;
    }

    return indices;
}

void VKRenderer::createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(this->physicalDevice);

    // to create device we need to set up queue create infos
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphicsFamily.value(),
        indices.presentFamily.value()
    };

    // each queue family has priority, we set it to 1.0f for now
    // currently only one queue per family is supported
    // so it is not necessary to set up a priority system
    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // device features currently empty for now
    VkPhysicalDeviceFeatures deviceFeatures{};

    // device extensions we need for swapchain
    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        #ifdef __APPLE__
        "VK_KHR_portability_subset",
        #endif
    };

    // Vulkan 1.3 features extension to enable dynamic rendering and synchronization2
    // add this with pNext to the device create info
    VkPhysicalDeviceVulkan13Features features13{};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features13.dynamicRendering = VK_TRUE;
    features13.synchronization2 = VK_TRUE;

    // device create info
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = &features13;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(this->physicalDevice, &createInfo, nullptr, &this->logicalDevice) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device");
    }

    // get the queues for the device, 3rd parameter is the queue index within the family
    // since we only requested 1 queue per family, it's always 0
    // initialize graphicsQueue
    vkGetDeviceQueue(this->logicalDevice, indices.graphicsFamily.value(), 0, &this->graphicsQueue);

    // initialize presentQueue
    vkGetDeviceQueue(this->logicalDevice, indices.presentFamily.value(), 0, &this->presentQueue);

    #if !defined(NDEBUG)
    std::cout << "Logical device created successfully" << std::endl;
    #endif
}

void VKRenderer::destroyLogicalDevice() {
    vkDestroyDevice(this->logicalDevice, nullptr);
    #if !defined(NDEBUG)
    std::cout << "Logical device destroyed successfully" << std::endl;
    #endif
}

void VKRenderer::createSurface() {
    if (!SDL_Vulkan_CreateSurface(this->window, this->instance, &this->surface)) {
        throw std::runtime_error("Failed to create Vulkan surface");
    }
    #if !defined(NDEBUG)
    std::cout << "Surface created successfully" << std::endl;
    #endif
}

void VKRenderer::destroySurface() {
    vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
    #if !defined(NDEBUG)
    std::cout << "Surface destroyed successfully" << std::endl;
    #endif
}

VKRenderer::SwapchainSupportDetails VKRenderer::querySwapchainSupport(VkPhysicalDevice device) {
    SwapchainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, this->surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, this->surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, this->surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, this->surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, this->surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR VKRenderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
    for (const auto& format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format; // return the first format that matches the preferred format
        }
    }
    return formats[0]; // return the first format if no preferred format is found
}

VkPresentModeKHR VKRenderer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& modes) {
    for (const auto& mode : modes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return mode; // return the first mode that matches the preferred mode
        }
    }

    // return the FIFO mode as fallback (vsync)
    #if !defined(NDEBUG)
    std::cout << "Using FIFO present mode (vsync)" << std::endl;
    #endif
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VKRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {

    // if the current extent is not max uint32_t, return it, it has been set by the window manager
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    // get the drawable size from SDL
    int width, height;
    SDL_Vulkan_GetDrawableSize(window, &width, &height);

    VkExtent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    // clamp the extent to the min and max extent
    actualExtent.width = std::clamp(actualExtent.width,
        capabilities.minImageExtent.width,
        capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height,
        capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height);

    return actualExtent;

}

void VKRenderer::createSwapchain() {
    // query the swapchain support details
    SwapchainSupportDetails swapchainSupport = querySwapchainSupport(this->physicalDevice);

    // choose the surface format
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats);

    // choose the present mode
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.presentModes);

    // choose the extent
    VkExtent2D extent = chooseSwapExtent(swapchainSupport.capabilities);

    uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount) {
        // if maxImageCount is 0, there is no limit on the number of images
        // if maxImageCount is not 0, we need to clamp the imageCount to the maxImageCount
        imageCount = swapchainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = this->surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(this->physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(this->logicalDevice, &createInfo, nullptr, &this->swapchain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swapchain");
    }

    // get the images in the swapchain
    vkGetSwapchainImagesKHR(this->logicalDevice, this->swapchain, &imageCount, nullptr);
    this->swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(this->logicalDevice, this->swapchain, &imageCount, this->swapchainImages.data());

    this->swapchainImageFormat = surfaceFormat.format;
    this->swapchainExtent = extent;

    #if !defined(NDEBUG)
    std::cout << "Swapchain created successfully" << std::endl;
    #endif
}

void VKRenderer::destroySwapchain() {
}
