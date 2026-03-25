# Vulkan Tutorial for Dive Engine — Part 1: Drawing a Triangle

This tutorial follows [vulkan-tutorial.com](https://vulkan-tutorial.com/), adapted for the Dive Engine (SDL2 instead of GLFW, building up `VKRenderer`). By the end of Part 1 you will have a hardcoded colorful triangle on screen with validation layers, proper synchronization, and frames in flight.

## Table of Contents

**Introduction**
1. [Vulkan vs SDL Renderer: Mental Model Shift](#1-vulkan-vs-sdl-renderer-mental-model-shift)
2. [Vulkan Architecture Overview](#2-vulkan-architecture-overview)

**Setup**
3. [Base Code](#3-base-code)
4. [Instance Creation](#4-instance-creation)
5. [Validation Layers](#5-validation-layers)
6. [Physical Devices and Queue Families](#6-physical-devices-and-queue-families)
7. [Logical Device and Queues](#7-logical-device-and-queues)

**Presentation**
8. [Window Surface](#8-window-surface)
9. [Swap Chain](#9-swap-chain)
10. [Image Views](#10-image-views)

**Graphics Pipeline Basics**
11. [Introduction to the Graphics Pipeline](#11-introduction-to-the-graphics-pipeline)
12. [Shader Modules](#12-shader-modules)
13. [Fixed Functions](#13-fixed-functions)
14. [Render Passes](#14-render-passes)
15. [Pipeline Conclusion](#15-pipeline-conclusion)

**Drawing**
16. [Framebuffers](#16-framebuffers)
17. [Command Buffers](#17-command-buffers)
18. [Rendering and Presentation](#18-rendering-and-presentation)
19. [Frames in Flight](#19-frames-in-flight)
20. [Full Code Checkpoint](#20-full-code-checkpoint)

---

## 1. Vulkan vs SDL Renderer: Mental Model Shift

### Your Current SDLRenderer Flow

```cpp
SDL_CreateWindow()      // Create window
SDL_CreateRenderer()    // Create renderer (GPU context hidden)
SDL_RenderClear()       // Clear screen
SDL_RenderCopy()        // Draw texture
SDL_RenderPresent()     // Show frame
```

SDL hides all GPU details. You say "draw this texture here" and it happens.

### Vulkan Flow

```cpp
vkCreateInstance()           // Connect to Vulkan driver
SDL_Vulkan_CreateSurface()   // Create drawable surface
vkCreateDevice()             // Create logical GPU connection
vkCreateSwapchainKHR()       // Create image buffers for double/triple buffering
vkCreateRenderPass()         // Define how rendering works
vkCreateGraphicsPipelines()  // Create shader pipeline
vkCreateFramebuffer()        // Connect swapchain images to render pass
vkAllocateCommandBuffers()   // Allocate command recording space

// Per frame:
vkAcquireNextImageKHR()      // Get next swapchain image
vkBeginCommandBuffer()       // Start recording commands
vkCmdBeginRenderPass()       // Begin render pass
vkCmdBindPipeline()          // Bind shader pipeline
vkCmdDraw()                  // Record draw command
vkCmdEndRenderPass()         // End render pass
vkEndCommandBuffer()         // Stop recording
vkQueueSubmit()              // Submit to GPU
vkQueuePresentKHR()          // Present to screen
```

**Key difference:** In SDL you call "draw" and it draws immediately. In Vulkan you *record* commands into a buffer, then *submit* that buffer to the GPU.

### Why So Verbose?

Vulkan gives you control over:
- Memory allocation (where textures/buffers live)
- Synchronization (when GPU/CPU wait for each other)
- Pipeline state (shaders, blend modes, depth testing)
- Command recording (batch many draws efficiently)

This verbosity enables better performance, predictable behavior, and cross-platform consistency.

---

## 2. Vulkan Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                         VkInstance                               │
│  (Connection to Vulkan driver, enables extensions)               │
└─────────────────────────────────────────────────────────────────┘
                                │
                ┌───────────────┴───────────────┐
                ▼                               ▼
┌───────────────────────────┐   ┌───────────────────────────────┐
│     VkPhysicalDevice      │   │        VkSurfaceKHR           │
│  (Your GPU - read-only)   │   │  (Window's drawable surface)  │
└───────────────────────────┘   └───────────────────────────────┘
                │                               │
                └───────────────┬───────────────┘
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                          VkDevice                                │
│  (Logical device - your interface to the GPU)                   │
│                                                                  │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐  │
│  │ VkQueue     │  │ VkQueue     │  │ VkSwapchainKHR          │  │
│  │ (Graphics)  │  │ (Present)   │  │ (Double/triple buffer)  │  │
│  └─────────────┘  └─────────────┘  └─────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
                                │
                ┌───────────────┼───────────────┐
                ▼               ▼               ▼
┌─────────────────┐ ┌─────────────────┐ ┌─────────────────────────┐
│  VkRenderPass   │ │   VkPipeline    │ │    VkCommandPool        │
│  (How to render)│ │ (Shader config) │ │  (Command allocation)   │
└─────────────────┘ └─────────────────┘ └─────────────────────────┘
```

### Object Relationships

| Object | What It Is | Lifetime |
|--------|------------|----------|
| `VkInstance` | Connection to Vulkan | App lifetime |
| `VkSurfaceKHR` | Window's drawable area | App lifetime |
| `VkPhysicalDevice` | Your GPU (read-only handle) | App lifetime |
| `VkDevice` | Logical GPU connection | App lifetime |
| `VkQueue` | Command submission endpoint | Device lifetime |
| `VkSwapchainKHR` | Image buffers for presenting | Can be recreated (resize) |
| `VkRenderPass` | Describes render operation | Pipeline lifetime |
| `VkPipeline` | Shader + state config | App lifetime (usually) |
| `VkCommandBuffer` | Recorded GPU commands | Per-frame or reusable |

---

# Setup

## 3. Base Code

Our `VKRenderer` class mirrors the `SDLRenderer` interface: `initialize`, `cleanup`, `beginFrame`, `endFrame`. We'll build it up incrementally throughout this tutorial.

```cpp
// VKRenderer.h
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

    bool isInitialized() const { return initialized; }

private:
    SDL_Window* window = nullptr;
    bool initialized = false;
};

#endif
```

```cpp
// VKRenderer.cpp
#include "VKRenderer.h"
#include "SDL_vulkan.h"

#include <iostream>
#include <stdexcept>

void VKRenderer::initialize(SDL_Window* win) {
    window = win;
    // We'll add create*() calls here as we go
    initialized = true;
    std::cout << "VKRenderer initialized!" << std::endl;
}

void VKRenderer::cleanup() {
    // We'll add destroy calls here (in reverse order of creation)
}

void VKRenderer::beginFrame() {}
void VKRenderer::endFrame() {}
```

When creating the SDL window, use the `SDL_WINDOW_VULKAN` flag:

```cpp
SDL_Window* window = SDL_CreateWindow(
    "Dive Engine",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    800, 600,
    SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN
);
```

### Compare to SDLRenderer

```cpp
// SDLRenderer creates its GPU context implicitly:
SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

// VKRenderer will create everything explicitly in initialize()
```

---

## 4. Instance Creation

The instance connects your app to the Vulkan driver and specifies which extensions you need.

### Code

Add to `VKRenderer.h` (private section):

```cpp
VkInstance instance = VK_NULL_HANDLE;

void createInstance();
```

Add to `VKRenderer.cpp`:

```cpp
#include <vector>
#include <cstring>
```

The first thing `createInstance` does is fill in a `VkApplicationInfo` struct. Most of these fields are informational metadata that drivers *may* use for optimization (e.g., well-known engines might get driver-specific fast paths), but `apiVersion` is the important one — it tells Vulkan which version of the API your app expects. We use `VK_API_VERSION_1_2` since it's widely supported and includes features we'll want later:

```cpp
void VKRenderer::createInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Dive Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Dive Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;
```

Next we need to find out which Vulkan extensions are required. Vulkan is modular — the core API doesn't know about windows or surfaces. SDL knows which platform-specific surface extensions are needed (e.g., `VK_KHR_surface` plus `VK_KHR_xlib_surface` on Linux X11, or `VK_MVK_macos_surface` on macOS). We use the two-call enumerate pattern: first call with `nullptr` to get the count, then call again with a sized vector to get the names:

```cpp
    unsigned int extensionCount = 0;
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr);
    std::vector<const char*> extensions(extensionCount);
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensions.data());

    #ifdef __APPLE__
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    extensions.push_back("VK_KHR_get_physical_device_properties2");
    #endif
```

On macOS, Vulkan runs on top of Metal via MoltenVK, which is a "portability subset" — not full Vulkan. The two extra extensions tell the loader to enumerate MoltenVK as a valid device despite not being fully conformant.

Now we assemble the `VkInstanceCreateInfo`. This is the pattern you'll see throughout Vulkan: a create-info struct that bundles everything the `vkCreate*` function needs. Every Vulkan struct starts with `sType` so the driver can identify and validate it:

```cpp
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledLayerCount = 0;

    #ifdef __APPLE__
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    #endif

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance");
    }
}
```

The second parameter to `vkCreateInstance` is a custom memory allocator — passing `nullptr` uses the default. The function returns `VK_SUCCESS` or an error code; Vulkan never throws exceptions, so we check the result explicitly.

Update `initialize()` and `cleanup()`:

```cpp
void VKRenderer::initialize(SDL_Window* win) {
    window = win;
    createInstance();
    initialized = true;
}

void VKRenderer::cleanup() {
    vkDestroyInstance(instance, nullptr);
}
```

### Key Concepts

**`VkApplicationInfo`**: Metadata about your app. Optional but helps drivers optimize.

**Extensions**: Vulkan is modular. The base API is minimal; extensions add features:
- `VK_KHR_surface` — window surfaces (SDL requests this automatically)
- `VK_KHR_portability_enumeration` — needed for MoltenVK on macOS

**`sType`**: Every Vulkan struct has a `sType` field. This enables the driver to validate and version structures.

**`nullptr` allocator**: The last parameter to most `vkCreate*` functions is a custom allocator. We pass `nullptr` to use default allocation.

---

## 5. Validation Layers

### What Are Validation Layers?

Vulkan's API does minimal error checking by default — even passing null pointers or invalid enums won't produce an error, just crashes or undefined behavior. Validation layers are optional components that hook into Vulkan function calls to catch mistakes:

- Checking parameter values against the specification
- Tracking object creation/destruction to find resource leaks
- Logging every call for profiling
- Detecting thread-safety issues

You enable them during development and disable them for release builds — zero overhead in production.

### Compare to SDL

```
SDL:    SDL_GetError() returns a string after something fails
Vulkan: Validation layers actively intercept calls and warn BEFORE crashes
```

### Code

Add to `VKRenderer.h` (private section):

```cpp
VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

void setupDebugMessenger();
bool checkValidationLayerSupport();
void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);
```

Add to `VKRenderer.cpp`:

First, we use a preprocessor check to enable validation layers only in debug builds. `NDEBUG` is defined by CMake in Release mode. We also declare which layers to enable — `VK_LAYER_KHRONOS_validation` is the unified validation layer from the Vulkan SDK that catches virtually all API misuse:

```cpp
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};
```

The debug messenger API (`vkCreateDebugUtilsMessengerEXT` / `vkDestroyDebugUtilsMessengerEXT`) is provided by an extension, not core Vulkan. Extension functions aren't loaded into the Vulkan dispatch table automatically — you have to look them up at runtime using `vkGetInstanceProcAddr`. These proxy functions handle that lookup and forward the call:

```cpp
VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}
```

`vkGetInstanceProcAddr` takes the function name as a string and returns a function pointer (or `nullptr` if the extension isn't available). The `PFN_` prefix is Vulkan's naming convention for function pointer types.

Next, the debug callback — this is the function Vulkan calls whenever a validation layer has something to report. The severity levels form a hierarchy: `VERBOSE` < `INFO` < `WARNING` < `ERROR`. We filter to warnings and above because verbose messages are extremely noisy. Returning `VK_FALSE` tells Vulkan to continue execution; returning `VK_TRUE` would abort the call that triggered the message:

```cpp
VKAPI_ATTR VkBool32 VKAPI_CALL VKRenderer::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
    }
    return VK_FALSE;
}
```

Before enabling validation layers, we check that the requested layers are actually installed on the system. This uses the familiar two-call enumerate pattern — first get the count, then fill the vector. We compare each requested layer name against the available layers:

```cpp
bool VKRenderer::checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool found = false;
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    return true;
}
```

If this returns `false`, you likely need to install the Vulkan SDK validation layers. On Linux: `sudo apt install vulkan-validationlayers-dev`.

`populateDebugMessengerCreateInfo` configures which messages we want to receive. We separate this into its own function because we'll reuse it in two places — once for the standalone debug messenger, and once embedded in the instance create info (explained below). The `messageSeverity` flags control which severity levels trigger the callback. The `messageType` flags control which categories of messages to report:
- `GENERAL` — events unrelated to the spec or performance
- `VALIDATION` — spec violations (the most useful category)
- `PERFORMANCE` — non-optimal Vulkan usage

```cpp
void VKRenderer::populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
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
}
```

Finally, `setupDebugMessenger` creates the actual debug messenger object using the proxy function from earlier:

```cpp
void VKRenderer::setupDebugMessenger() {
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("Failed to set up debug messenger");
    }
}
```

Now update `createInstance()` to enable validation layers and the debug utils extension. The first change is a safety check — if validation was requested but the layers aren't installed, fail fast with a clear error:

```cpp
void VKRenderer::createInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("Validation layers requested but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Dive Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Dive Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;
```

When validation is enabled, we add the `VK_EXT_debug_utils` extension to our extension list. This is the extension that provides the debug messenger API:

```cpp
    unsigned int sdlExtensionCount = 0;
    SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, nullptr);
    std::vector<const char*> extensions(sdlExtensionCount);
    SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, extensions.data());

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    #ifdef __APPLE__
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    extensions.push_back("VK_KHR_get_physical_device_properties2");
    #endif
```

The key new piece is the `pNext` chain. Every Vulkan create-info struct has a `pNext` field that can point to another struct, forming a linked list of configuration. By attaching a `VkDebugUtilsMessengerCreateInfoEXT` to the instance create info's `pNext`, the validation layer will also debug the `vkCreateInstance` and `vkDestroyInstance` calls themselves — which would otherwise happen before/after the debug messenger exists:

```cpp
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    #ifdef __APPLE__
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    #endif

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance");
    }
}
```

Update `initialize()` and `cleanup()`:

```cpp
void VKRenderer::initialize(SDL_Window* win) {
    window = win;
    createInstance();
    setupDebugMessenger();
    initialized = true;
}

void VKRenderer::cleanup() {
    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }
    vkDestroyInstance(instance, nullptr);
}
```

### Summary

We now have a full debug pipeline: validation layers intercept every Vulkan call and report misuse through our callback. The proxy functions handle loading the extension API, the `pNext` chain catches errors during instance creation/destruction, and the `#ifdef NDEBUG` guard ensures zero overhead in release builds.

---

## 6. Physical Devices and Queue Families

A physical device represents your GPU. You query its capabilities and choose one that supports what you need.

### Queue Families

GPUs have different types of queues:
- **Graphics queue**: Rendering commands
- **Compute queue**: Compute shaders
- **Transfer queue**: Memory copies
- **Present queue**: Presenting to screen

Often the graphics queue can also present, but not always. We need to find queue families that support both.

### Code

Add to `VKRenderer.h` (private section):

```cpp
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

void pickPhysicalDevice();
bool isDeviceSuitable(VkPhysicalDevice device);
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
```

Add to `VKRenderer.cpp`:

`pickPhysicalDevice` uses the enumerate pattern to list all GPUs, then picks the first one that meets our requirements. `VkPhysicalDevice` is a read-only handle — you don't create or destroy it, you just pick one from the available set. After selection, we print the GPU name so you can verify the right device was chosen (useful on systems with both integrated and discrete GPUs):

```cpp
void VKRenderer::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("No GPUs with Vulkan support found");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("No suitable GPU found");
    }

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physicalDevice, &props);
    std::cout << "Selected GPU: " << props.deviceName << std::endl;
}
```

`findQueueFamilies` is more nuanced. A GPU exposes its capabilities through *queue families* — groups of queues that support certain operations. We need at least one family that supports graphics commands (`VK_QUEUE_GRAPHICS_BIT`) and one that can present to our window surface. Often these are the same family, but on some hardware they differ.

We use `std::optional<uint32_t>` because queue family index 0 is valid, so we can't use 0 or -1 as a "not found" sentinel. `std::optional` cleanly distinguishes "no value" from "value is 0":

```cpp
VKRenderer::QueueFamilyIndices VKRenderer::findQueueFamilies(VkPhysicalDevice dev) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) break;
        i++;
    }

    return indices;
}
```

Note that `vkGetPhysicalDeviceSurfaceSupportKHR` checks whether a specific queue family can present to a specific surface — this is a property of the queue-family-and-surface *pair*, not just the queue family alone.

`isDeviceSuitable` is deliberately minimal for now — it only checks that the required queue families exist. We'll add swapchain support checks in Section 9:

```cpp
bool VKRenderer::isDeviceSuitable(VkPhysicalDevice dev) {
    QueueFamilyIndices indices = findQueueFamilies(dev);
    return indices.isComplete();
}
```

> **Note:** `findQueueFamilies` references `surface`, which we haven't created yet. We'll add the surface in Section 8. In the init order, surface creation comes before physical device selection.

### Key Concepts

**Enumeration pattern**: Vulkan uses a common two-call pattern for listing things:

```cpp
uint32_t count = 0;
vkEnumerateSomething(handle, &count, nullptr);      // Get count
std::vector<Thing> things(count);
vkEnumerateSomething(handle, &count, things.data()); // Fill array
```

**`std::optional`**: Queue family indices are `uint32_t` where any value (including 0) is valid. `std::optional` lets us distinguish "not found" from "found at index 0."

**Device suitability**: We check that the device has both a graphics queue and a present queue. We'll extend this check with swapchain support later.

---

## 7. Logical Device and Queues

The logical device is your interface to the GPU. Queues are where you submit commands.

### Code

Add to `VKRenderer.h` (private section):

```cpp
VkDevice device = VK_NULL_HANDLE;
VkQueue graphicsQueue = VK_NULL_HANDLE;
VkQueue presentQueue = VK_NULL_HANDLE;

void createLogicalDevice();
```

Add to `VKRenderer.cpp`:

```cpp
#include <set>
```

Creating a logical device starts by specifying which queues we want. We use a `std::set` to collect unique queue family indices — if the graphics and present families happen to be the same (very common), the set deduplicates them so we only create one `VkDeviceQueueCreateInfo`. Each queue gets a priority from 0.0 to 1.0 that influences scheduling when multiple queues compete for GPU time. With a single queue per family, priority doesn't matter, but Vulkan requires it:

```cpp
void VKRenderer::createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphicsFamily.value(),
        indices.presentFamily.value()
    };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
```

Next we declare which device features and extensions we need. `VkPhysicalDeviceFeatures` is a struct full of `VkBool32` fields for optional GPU capabilities (geometry shaders, tessellation, wide lines, etc.). We leave it zero-initialized for now — we'll enable `samplerAnisotropy` in Part 2 when we add textures.

`VK_KHR_swapchain` is the most important device extension — without it, you can't present rendered images to the screen. It's technically optional because Vulkan can also be used for off-screen rendering or compute-only workloads:

```cpp
    VkPhysicalDeviceFeatures deviceFeatures{};

    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        #ifdef __APPLE__
        "VK_KHR_portability_subset",
        #endif
    };
```

Now we assemble the device create info and create the logical device. Note that device-level validation layers are deprecated in modern Vulkan (validation is per-instance now), but we still set them for compatibility with older implementations:

```cpp
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
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

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device");
    }
```

Queues are created implicitly when you create the device — you don't call a separate `vkCreateQueue`. Instead, you retrieve handles to the queues that were created according to your `VkDeviceQueueCreateInfo` structs. The third parameter (0) is the queue index within that family — since we only requested 1 queue per family, it's always 0:

```cpp
    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}
```

### Key Concepts

**Queue priority**: 0.0 to 1.0, affects scheduling. Usually just use 1.0.

**Device extensions**: `VK_KHR_swapchain` is required for presenting to screen.

**Device features**: GPU capabilities like geometry shaders, multi-viewport, etc. We leave this empty for now.

**`std::set` for unique families**: If the graphics and present queue families are the same (common), we only create one queue create info.

### Compare to SDLRenderer

```cpp
// SDLRenderer: renderer implicitly creates GPU context
SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

// Vulkan: explicit device and queue creation
vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
vkGetDeviceQueue(device, queueFamily, 0, &graphicsQueue);
```

---

# Presentation

## 8. Window Surface

The surface connects Vulkan to your SDL window. This is where SDL helps — it handles all the platform-specific differences.

### Why Surface Before Physical Device?

The surface must exist before we select a physical device because `findQueueFamilies` needs it to check for present support. In `initialize()`, the call order is: `createInstance()` → `setupDebugMessenger()` → `createSurface()` → `pickPhysicalDevice()` → `createLogicalDevice()`.

### Code

Add to `VKRenderer.h` (private section):

```cpp
VkSurfaceKHR surface = VK_NULL_HANDLE;

void createSurface();
```

Add to `VKRenderer.cpp`:

```cpp
void VKRenderer::createSurface() {
    if (!SDL_Vulkan_CreateSurface(window, instance, &surface)) {
        throw std::runtime_error("Failed to create Vulkan surface: " +
                                 std::string(SDL_GetError()));
    }
}
```

### What SDL Hides

Without SDL, you'd need platform-specific code:

```cpp
// Linux (X11)
vkCreateXlibSurfaceKHR(instance, &createInfo, nullptr, &surface);

// Windows
vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface);

// macOS (Metal via MoltenVK)
vkCreateMetalSurfaceEXT(instance, &createInfo, nullptr, &surface);
```

`SDL_Vulkan_CreateSurface()` handles all of this for you.

### Compare to SDLRenderer

```cpp
// SDLRenderer: the window IS the surface
SDL_Window* window = SDL_CreateWindow(..., SDL_WINDOW_SHOWN);

// Vulkan: window and surface are separate
SDL_Window* window = SDL_CreateWindow(..., SDL_WINDOW_VULKAN);
SDL_Vulkan_CreateSurface(window, instance, &surface);
```

---

## 9. Swap Chain

The swap chain is a queue of images that get presented to the screen — double or triple buffering.

### Why a Swap Chain?

Without buffering, the GPU writes to the screen buffer while the display reads it, causing tearing. The swap chain provides multiple buffers so the GPU can write to a back buffer while the display reads the front buffer.

### Code

Add to `VKRenderer.h` (private section):

```cpp
struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

VkSwapchainKHR swapchain = VK_NULL_HANDLE;
std::vector<VkImage> swapchainImages;
VkFormat swapchainImageFormat;
VkExtent2D swapchainExtent;

SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device);
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& modes);
VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
void createSwapchain();
```

Add to `VKRenderer.cpp`:

```cpp
#include <algorithm>
#include <limits>
```

Creating a swapchain requires querying three things from the physical device: surface capabilities (min/max image count, min/max extent, supported transforms), supported surface formats (pixel format + color space), and supported present modes. `querySwapchainSupport` gathers all three using the enumerate pattern:

```cpp
VKRenderer::SwapchainSupportDetails VKRenderer::querySwapchainSupport(VkPhysicalDevice dev) {
    SwapchainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}
```

`chooseSwapSurfaceFormat` picks the pixel format and color space for swapchain images. `VK_FORMAT_B8G8R8A8_SRGB` means 8 bits per channel in BGRA order with SRGB gamma encoding — this gives gamma-correct rendering where colors look the same across different monitors. `VK_COLOR_SPACE_SRGB_NONLINEAR_KHR` is the standard sRGB color space. If our preferred combo isn't available, we fall back to whatever the driver offers first:

```cpp
VkSurfaceFormatKHR VKRenderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
    for (const auto& format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }
    return formats[0];
}
```

`chooseSwapPresentMode` selects how images are presented to the screen. The available modes have different latency and tearing characteristics:

| Mode | Description |
|------|-------------|
| `FIFO` | VSync — guaranteed available on all hardware, no tearing, but adds latency |
| `MAILBOX` | Triple buffer — GPU keeps rendering, only the newest image gets displayed. Low latency, no tearing |
| `IMMEDIATE` | No waiting at all — lowest latency but may tear |

We prefer `MAILBOX` for its low-latency-without-tearing property. `FIFO` is the safe fallback since the Vulkan spec guarantees it's always supported:

```cpp
VkPresentModeKHR VKRenderer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& modes) {
    for (const auto& mode : modes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return mode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}
```

`chooseSwapExtent` determines the resolution of the swapchain images. Most window managers set `currentExtent` to the window size in pixels, and we simply use that. But some (notably Wayland) set it to `uint32_max` to signal "you decide." In that case we query the actual drawable size from SDL and clamp it to the GPU's supported range:

```cpp
VkExtent2D VKRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    int width, height;
    SDL_Vulkan_GetDrawableSize(window, &width, &height);

    VkExtent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    actualExtent.width = std::clamp(actualExtent.width,
        capabilities.minImageExtent.width,
        capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height,
        capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height);

    return actualExtent;
}
```

Now `createSwapchain` ties everything together. First we query support and pick our preferences. We request `minImageCount + 1` images — one more than the minimum gives us an extra buffer to work with while waiting for the driver, enabling smoother pipelining:

```cpp
void VKRenderer::createSwapchain() {
    SwapchainSupportDetails support = querySwapchainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(support.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(support.presentModes);
    VkExtent2D extent = chooseSwapExtent(support.capabilities);

    uint32_t imageCount = support.capabilities.minImageCount + 1;
    if (support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount) {
        imageCount = support.capabilities.maxImageCount;
    }
```

The create info has several fields worth understanding. `imageArrayLayers` is 1 for normal rendering (2 for stereoscopic 3D). `imageUsage` tells Vulkan how we'll use these images — `COLOR_ATTACHMENT_BIT` means we render directly to them. If we wanted post-processing, we'd add `TRANSFER_DST_BIT` to blit into them instead:

```cpp
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
```

If the graphics and present queue families are different, images must be shared between them. `CONCURRENT` mode lets multiple families access the image without explicit ownership transfers (simpler but slightly slower). `EXCLUSIVE` mode gives one family exclusive access (faster, since it's the common case where both queues are the same family):

```cpp
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
```

`preTransform` applies a rotation/mirror to the image before presentation — we use `currentTransform` to apply no extra transform. `compositeAlpha` controls alpha blending with other windows — `OPAQUE` means our window is fully opaque. `clipped = VK_TRUE` lets the driver discard pixels obscured by other windows (a free optimization). `oldSwapchain` is used during swapchain recreation (Part 3) to hand off the old one:

```cpp
    createInfo.preTransform = support.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swap chain");
    }
```

Finally, we retrieve handles to the swapchain images. The driver creates them — we just get `VkImage` handles. We also save the format and extent since we'll need them later for image views, framebuffers, and the render pass:

```cpp
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

    swapchainImageFormat = surfaceFormat.format;
    swapchainExtent = extent;
}
```

Also update `isDeviceSuitable` to verify the device supports at least one format and one present mode — a device with an empty format or present mode list can't create a swapchain:

```cpp
bool VKRenderer::isDeviceSuitable(VkPhysicalDevice dev) {
    QueueFamilyIndices indices = findQueueFamilies(dev);

    SwapchainSupportDetails swapchainSupport = querySwapchainSupport(dev);
    bool swapchainAdequate = !swapchainSupport.formats.empty() &&
                             !swapchainSupport.presentModes.empty();

    return indices.isComplete() && swapchainAdequate;
}
```

---

## 10. Image Views

Images can't be accessed directly in pipelines. You need a `VkImageView` that describes how to interpret the image data.

### Code

Add to `VKRenderer.h` (private section):

```cpp
std::vector<VkImageView> swapchainImageViews;

void createImageViews();
```

Add to `VKRenderer.cpp`:

We create one `VkImageView` per swapchain image. The `viewType` determines how the image data is interpreted — `2D` for regular textures, `CUBE` for cubemaps, `2D_ARRAY` for texture arrays.

The `components` field lets you remap color channels (e.g., swapping R and B, or broadcasting one channel to all four). `SWIZZLE_IDENTITY` means "use the channel as-is" — the default you want for normal rendering:

```cpp
void VKRenderer::createImageViews() {
    swapchainImageViews.resize(swapchainImages.size());

    for (size_t i = 0; i < swapchainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapchainImageFormat;

        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
```

The `subresourceRange` describes which part of the image the view covers. `aspectMask` selects color vs depth vs stencil data. `baseMipLevel` and `levelCount` select mip levels (we have no mipmaps yet, so just level 0). `baseArrayLayer` and `layerCount` select array layers (we're not using texture arrays, so just layer 0):

```cpp
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &createInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image view");
        }
    }
}
```

---

# Graphics Pipeline Basics

## 11. Introduction to the Graphics Pipeline

The graphics pipeline is the sequence of operations that transform vertices into pixels on screen:

```
Vertex Data
    ↓
┌──────────────────┐
│  Input Assembler  │  ← Collects vertices, uses index buffer
└──────────────────┘
    ↓
┌──────────────────┐
│  Vertex Shader    │  ← Transforms positions (model → screen space)  [PROGRAMMABLE]
└──────────────────┘
    ↓
┌──────────────────┐
│  Rasterization    │  ← Converts triangles to pixel fragments        [FIXED]
└──────────────────┘
    ↓
┌──────────────────┐
│  Fragment Shader  │  ← Colors each pixel fragment                   [PROGRAMMABLE]
└──────────────────┘
    ↓
┌──────────────────┐
│  Color Blending   │  ← Combines fragments with framebuffer          [FIXED]
└──────────────────┘
    ↓
  Framebuffer
```

**Vulkan pipelines are immutable** — you can't change settings after creation. If you need different shaders or blend modes, create a separate pipeline. This allows the driver to optimize aggressively.

### Compare to SDL

SDL has no pipeline concept. You just call `SDL_RenderCopy` with whatever texture and blend mode you want. In Vulkan, all rendering state must be baked into a pipeline object upfront.

---

## 12. Shader Modules

Vulkan uses SPIR-V bytecode, not GLSL directly. You write GLSL, then compile to SPIR-V with `glslc`.

For Part 1, we hardcode the triangle vertices directly in the vertex shader (matching the official tutorial). We'll move to vertex buffers in Part 2.

### Vertex Shader (`resources/shaders/hardcoded_triangle.vert`)

```glsl
#version 450

layout(location = 0) out vec3 fragColor;

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}
```

### Fragment Shader (`resources/shaders/hardcoded_triangle.frag`)

```glsl
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
```

### Compile to SPIR-V

```bash
glslc resources/shaders/hardcoded_triangle.vert -o resources/shaders/hardcoded_triangle.vert.spv
glslc resources/shaders/hardcoded_triangle.frag -o resources/shaders/hardcoded_triangle.frag.spv
```

### Loading Shader Code

Add to `VKRenderer.h` (private section):

```cpp
static std::vector<char> readFile(const std::string& filename);
VkShaderModule createShaderModule(const std::vector<char>& code);
```

Add to `VKRenderer.cpp`:

```cpp
#include <fstream>

std::vector<char> VKRenderer::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

VkShaderModule VKRenderer::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module");
    }

    return shaderModule;
}
```

### Key Concepts

**`gl_VertexIndex`**: Built-in variable that tells the vertex shader which vertex is being processed (0, 1, or 2 for our triangle).

**Shader modules are temporary**: You create them to build the pipeline, then destroy them immediately after. The pipeline keeps its own copy of the compiled code.

**SPIR-V**: An intermediate representation that all Vulkan drivers understand. Write GLSL, compile once, run anywhere.

---

## 13. Fixed Functions

The non-programmable pipeline stages are configured through structs. Even though vertices are hardcoded in the shader, we still need to configure these stages.

All of the following code goes inside `createGraphicsPipeline()`, which we'll assemble in Section 15.

### Vertex Input (empty for now)

Since our vertices are hardcoded in the shader, we tell the pipeline there's no vertex input:

```cpp
VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
vertexInputInfo.vertexBindingDescriptionCount = 0;
vertexInputInfo.vertexAttributeDescriptionCount = 0;
```

### Input Assembly

How vertices form primitives:

```cpp
VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
inputAssembly.primitiveRestartEnable = VK_FALSE;
```

### Dynamic State

Viewport and scissor are set at draw time so we don't need to recreate the pipeline on window resize:

```cpp
std::vector<VkDynamicState> dynamicStates = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
};

VkPipelineDynamicStateCreateInfo dynamicState{};
dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
dynamicState.pDynamicStates = dynamicStates.data();

VkPipelineViewportStateCreateInfo viewportState{};
viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
viewportState.viewportCount = 1;
viewportState.scissorCount = 1;
```

### Rasterizer

The rasterizer converts the triangles defined by the vertex shader into fragments (pixel-sized pieces) that the fragment shader will color. Key fields:
- `depthClampEnable` — if `VK_TRUE`, fragments beyond the near/far planes are clamped instead of discarded (useful for shadow maps, requires a GPU feature)
- `rasterizerDiscardEnable` — if `VK_TRUE`, geometry never passes through rasterization, effectively disabling output (useful for transform feedback)
- `polygonMode` — `FILL` fills triangles, `LINE` draws wireframe, `POINT` draws vertices only
- `cullMode` — `BACK_BIT` discards back-facing triangles (a major performance optimization)
- `frontFace` — defines which winding order is "front." We'll change this to `COUNTER_CLOCKWISE` in Part 3 when we add 3D models with GLM

```cpp
VkPipelineRasterizationStateCreateInfo rasterizer{};
rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
rasterizer.depthClampEnable = VK_FALSE;
rasterizer.rasterizerDiscardEnable = VK_FALSE;
rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
rasterizer.lineWidth = 1.0f;
rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
rasterizer.depthBiasEnable = VK_FALSE;
```

### Multisampling (disabled)

Multisampling (MSAA) smooths jagged edges by sampling each pixel at multiple points. We disable it for now (`SAMPLE_COUNT_1_BIT` = no multisampling). Part 4 discusses how to enable it later:

```cpp
VkPipelineMultisampleStateCreateInfo multisampling{};
multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
multisampling.sampleShadingEnable = VK_FALSE;
multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
```

### Color Blending

Color blending controls how the fragment shader's output combines with the existing framebuffer contents. There are two levels of configuration: per-attachment (one per color attachment in the render pass) and global.

The per-attachment struct controls whether blending is enabled and which channels to write. With `blendEnable = VK_FALSE`, the fragment color replaces the framebuffer value. The `colorWriteMask` controls which channels are actually written — we want all four (RGBA):

```cpp
VkPipelineColorBlendAttachmentState colorBlendAttachment{};
colorBlendAttachment.colorWriteMask =
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
colorBlendAttachment.blendEnable = VK_FALSE;
```

The global struct aggregates all per-attachment states. `logicOpEnable` would replace blending with bitwise logic operations (rarely used in normal rendering):

```cpp
VkPipelineColorBlendStateCreateInfo colorBlending{};
colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
colorBlending.logicOpEnable = VK_FALSE;
colorBlending.attachmentCount = 1;
colorBlending.pAttachments = &colorBlendAttachment;
```

### Pipeline Layout (empty for now)

We'll add push constants in Part 2. For now, it's empty:

```cpp
VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
pipelineLayoutInfo.setLayoutCount = 0;
pipelineLayoutInfo.pushConstantRangeCount = 0;
```

---

## 14. Render Passes

A render pass describes what attachments (color, depth) are used, how they're loaded/stored, and what subpasses exist.

### Code

Add to `VKRenderer.h` (private section):

```cpp
VkRenderPass renderPass = VK_NULL_HANDLE;

void createRenderPass();
```

Add to `VKRenderer.cpp`:

First we describe the color attachment — the swapchain image we'll render into. `loadOp` controls what happens to the attachment at the start of the render pass: `CLEAR` fills it with a color (we'll specify the clear color when recording commands). `storeOp = STORE` means we keep the rendered contents (as opposed to `DONT_CARE` which discards them — useful for depth buffers).

The `initialLayout` is the layout the image is in when the render pass begins. `UNDEFINED` means we don't care about previous contents (the clear will overwrite everything anyway). `finalLayout` is what the driver transitions the image to after the render pass — `PRESENT_SRC_KHR` makes it ready for presentation to the screen:

```cpp
void VKRenderer::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
```

A subpass is a rendering operation within the render pass. Even though we only have one subpass, Vulkan requires you to define it explicitly. The attachment reference connects this subpass to attachment 0 (our color attachment) and specifies the optimal layout for writing color data. The `layout(location = 0) out vec4 outColor` in the fragment shader maps to `pColorAttachments[0]`:

```cpp
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
```

Subpass dependencies define execution ordering between subpasses (or between a subpass and external commands). `VK_SUBPASS_EXTERNAL` refers to the implicit operations that happen before/after the render pass — in this case, the swapchain image layout transition. This dependency says: "don't start writing to the color attachment until the image is available from the swapchain." Without it, we might try to render into an image the presentation engine is still reading:

```cpp
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass");
    }
}
```

### Key Concepts

**Load/Store operations**:

| loadOp | Description |
|--------|-------------|
| `CLEAR` | Clear to a value at start |
| `LOAD` | Preserve existing contents |
| `DONT_CARE` | Contents undefined (fastest) |

| storeOp | Description |
|---------|-------------|
| `STORE` | Keep results |
| `DONT_CARE` | Contents undefined after |

**Image layouts**: Images must be in specific layouts for different operations:
- `UNDEFINED` — don't care about previous contents
- `COLOR_ATTACHMENT_OPTIMAL` — best for rendering to
- `PRESENT_SRC_KHR` — ready for presentation

**Subpass dependency**: `VK_SUBPASS_EXTERNAL` refers to commands before the render pass. The dependency ensures the swap chain image transition happens before we try to write to it.

---

## 15. Pipeline Conclusion

Now we assemble everything from the previous sections into `createGraphicsPipeline()`.

### Code

Add to `VKRenderer.h` (private section):

```cpp
VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
VkPipeline graphicsPipeline = VK_NULL_HANDLE;

void createGraphicsPipeline();
```

Add to `VKRenderer.cpp`:

The first step is loading the compiled SPIR-V bytecode and creating shader modules from it. Each shader stage gets a `VkPipelineShaderStageCreateInfo` that specifies the shader module and the entry point function name. `pName = "main"` refers to the `void main()` in our GLSL code — Vulkan supports multiple entry points per module, so you must specify which one to use:

```cpp
void VKRenderer::createGraphicsPipeline() {
    auto vertShaderCode = readFile("resources/shaders/hardcoded_triangle.vert.spv");
    auto fragShaderCode = readFile("resources/shaders/hardcoded_triangle.frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
```

Now we set up all the fixed-function state structs that were previewed in Section 13. Vertex input is empty because our vertices are hardcoded in the shader. `TRIANGLE_LIST` means every 3 vertices form an independent triangle:

```cpp
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
```

We mark viewport and scissor as dynamic state so we can set them per-frame in the command buffer rather than baking them into the pipeline. This means window resizes won't require pipeline recreation — a significant convenience:

```cpp
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
```

Rasterizer, multisampling, and color blending — the same configuration from Section 13:

```cpp
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
```

The pipeline layout defines the interface between shaders and the data they access — descriptor set layouts (for uniform buffers and textures) and push constant ranges (for small per-draw data). We leave it empty for now and add push constants in Part 2:

```cpp
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout");
    }
```

Finally, the `VkGraphicsPipelineCreateInfo` brings everything together — shader stages, all fixed-function state, layout, and render pass. The `subpass = 0` specifies which subpass in the render pass this pipeline will be used with. The second parameter to `vkCreateGraphicsPipelines` is an optional pipeline cache that speeds up pipeline creation across runs (we pass `VK_NULL_HANDLE` for now).

After creating the pipeline, we destroy the shader modules — the pipeline has its own copy of the compiled shader code, so the modules are no longer needed:

```cpp
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipeline");
    }

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}
```

---

# Drawing

## 16. Framebuffers

A framebuffer connects your render pass to actual images. Each swap chain image needs its own framebuffer.

### Code

Add to `VKRenderer.h` (private section):

```cpp
std::vector<VkFramebuffer> swapchainFramebuffers;

void createFramebuffers();
```

Add to `VKRenderer.cpp`:

```cpp
void VKRenderer::createFramebuffers() {
    swapchainFramebuffers.resize(swapchainImageViews.size());

    for (size_t i = 0; i < swapchainImageViews.size(); i++) {
        VkImageView attachments[] = { swapchainImageViews[i] };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapchainExtent.width;
        framebufferInfo.height = swapchainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapchainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer");
        }
    }
}
```

### Key Concepts

**One framebuffer per swap chain image**: If you have 3 swap chain images (triple buffering), you create 3 framebuffers.

**Attachment order**: The `pAttachments` array must match the order of attachments in the render pass. When you add a depth buffer later (Part 3), you'll pass two attachments.

---

## 17. Command Buffers

Commands in Vulkan aren't executed immediately. You record them into a command buffer, then submit that buffer to a queue.

### Code

Add to `VKRenderer.h` (private section):

```cpp
static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

VkCommandPool commandPool = VK_NULL_HANDLE;
std::vector<VkCommandBuffer> commandBuffers;

void createCommandPool();
void createCommandBuffers();
void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
```

Add to `VKRenderer.cpp`:

A command pool is a memory manager for command buffers. It's tied to a specific queue family — all command buffers allocated from this pool will be submitted to the graphics queue. The `RESET_COMMAND_BUFFER_BIT` flag allows us to re-record individual command buffers each frame (without it, you'd have to reset the entire pool at once):

```cpp
void VKRenderer::createCommandPool() {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = indices.graphicsFamily.value();

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool");
    }
}
```

We allocate one command buffer per frame in flight. `PRIMARY` level means these buffers are submitted directly to a queue. `SECONDARY` buffers can be called from within a primary buffer — useful for multi-threaded recording where different threads record different secondary buffers and a primary buffer assembles them:

```cpp
void VKRenderer::createCommandBuffers() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers");
    }
}
```

`recordCommandBuffer` is where the actual rendering commands go. First we begin the command buffer, then begin the render pass. The render pass needs to know which framebuffer to target (selected by `imageIndex` — the swapchain image we acquired) and the clear color to use (dark blueish gray). `VK_SUBPASS_CONTENTS_INLINE` means we'll record commands directly into this primary buffer rather than calling secondary buffers:

```cpp
void VKRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapchainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchainExtent;

    VkClearValue clearColor = {{{0.1f, 0.1f, 0.15f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
```

After beginning the render pass, we bind our graphics pipeline, then set the viewport and scissor dynamically (since we declared them as dynamic state in the pipeline). The viewport maps normalized device coordinates to pixel coordinates. The scissor defines which region of the framebuffer can be written to — pixels outside the scissor are discarded:

```cpp
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchainExtent.width);
    viewport.height = static_cast<float>(swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkScissor scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
```

Finally, the draw call and cleanup. `vkCmdDraw(commandBuffer, 3, 1, 0, 0)` means: draw 3 vertices, 1 instance, starting from vertex index 0 and instance index 0. Since our vertices are hardcoded in the shader, the vertex count is all Vulkan needs to know. After drawing, we end the render pass and finalize the command buffer:

```cpp
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer");
    }
}
```

### Compare to SDLRenderer

```
SDLRenderer: "Draw this now"
  SDL_RenderCopy(renderer, texture, &src, &dst);

Vulkan: "Record this command, I'll submit it later"
  vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
```

---

## 18. Rendering and Presentation

Now we implement `beginFrame()` and `endFrame()` — the core frame loop.

### Synchronization Primitives

| Primitive | Scope | Use Case |
|-----------|-------|----------|
| **Fence** | CPU ↔ GPU | CPU waits for GPU to finish a frame |
| **Semaphore** | GPU ↔ GPU | Signal between queue operations |

### The Frame Sync Flow

```
1. CPU: Wait for fence[N]          (GPU finished with this frame slot)
2. CPU: Acquire next swap chain image   (signals imageAvailable semaphore)
3. CPU: Reset and record commands
4. CPU: Submit to graphics queue
         Wait on:  imageAvailable       (don't render until image ready)
         Signal:   renderFinished       (rendering done)
         Signal:   fence[N]             (CPU can track completion)
5. CPU: Present the image
         Wait on:  renderFinished       (don't present until rendered)
```

### Code

Add to `VKRenderer.h` (private section):

```cpp
std::vector<VkSemaphore> imageAvailableSemaphores;
std::vector<VkSemaphore> renderFinishedSemaphores;
std::vector<VkFence> inFlightFences;
uint32_t currentFrame = 0;
uint32_t currentImageIndex = 0;

void createSyncObjects();
```

Add to `VKRenderer.cpp`:

We create one set of sync objects per frame in flight. Each set has: an `imageAvailable` semaphore (GPU signals when swapchain image is ready), a `renderFinished` semaphore (GPU signals when rendering is done), and a fence (CPU waits on it to know the frame slot is free).

`VK_FENCE_CREATE_SIGNALED_BIT` is critical — fences normally start unsignaled, meaning `vkWaitForFences` would block forever on the first frame since no previous GPU work exists to signal it. By starting signaled, the first frame's wait returns immediately:

```cpp
void VKRenderer::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create sync objects");
        }
    }
}
```

Now implement `beginFrame()`. The fence wait is the CPU-GPU synchronization point: it blocks until the GPU finishes using this frame slot's resources (command buffer, semaphores). Once the fence signals, we know the GPU is done and we can safely reset and reuse those resources. `vkAcquireNextImageKHR` asks the presentation engine for the next available swapchain image, and the `imageAvailable` semaphore will be signaled once the image is actually ready:

```cpp
void VKRenderer::beginFrame() {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    vkAcquireNextImageKHR(device, swapchain, UINT64_MAX,
        imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &currentImageIndex);

    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    recordCommandBuffer(commandBuffers[currentFrame], currentImageIndex);
}
```

`endFrame()` submits the recorded command buffer and presents the result. The submit info orchestrates GPU-GPU synchronization:
- `pWaitSemaphores` / `pWaitDstStageMask`: wait for `imageAvailable` before the color attachment output stage (don't write pixels until the image is ready)
- `pSignalSemaphores`: signal `renderFinished` when rendering completes
- The fence parameter signals `inFlightFences[currentFrame]` when the GPU finishes, so the next time this frame slot comes around, `beginFrame` can wait on it:

```cpp
void VKRenderer::endFrame() {
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer");
    }
```

Presentation waits on `renderFinished` to ensure the image is fully rendered before being displayed. `currentFrame` advances modulo `MAX_FRAMES_IN_FLIGHT`, rotating between frame slots so the CPU can prepare the next frame while the GPU works on the current one:

```cpp
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = { swapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &currentImageIndex;

    vkQueuePresentKHR(presentQueue, &presentInfo);

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
```

### Compare to SDLRenderer

```
SDLRenderer: SDL_RenderPresent handles everything
  SDL_RenderPresent(renderer);

Vulkan: you manage all synchronization
  vkWaitForFences(...)
  vkAcquireNextImageKHR(...)
  vkQueueSubmit(...)
  vkQueuePresentKHR(...)
```

---

## 19. Frames in Flight

The sync objects and double-buffering pattern from Section 18 implement "frames in flight." With `MAX_FRAMES_IN_FLIGHT = 2`:

- Frame slot 0 and frame slot 1 alternate
- Each has its own command buffer, semaphores, and fence
- The CPU can prepare the next frame while the GPU renders the current one
- `currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT` rotates between slots

This is already implemented above. The key insight is that `MAX_FRAMES_IN_FLIGHT` controls how far ahead the CPU can get. Setting it to 1 means the CPU blocks every frame. Setting it to 2 means the CPU can be one frame ahead.

---

## 20. Full Code Checkpoint

At this point, your `initialize()` and `cleanup()` should look like this:

### `VKRenderer.h`

```cpp
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

    bool isInitialized() const { return initialized; }

private:
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        bool isComplete() const {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct SwapchainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    SDL_Window* window = nullptr;
    bool initialized = false;

    // Vulkan core objects
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;

    // Swap chain
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;
    std::vector<VkImageView> swapchainImageViews;

    // Pipeline
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;

    // Drawing
    std::vector<VkFramebuffer> swapchainFramebuffers;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;

    // Sync
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;
    uint32_t currentImageIndex = 0;

    // Setup
    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();

    // Presentation
    void createSwapchain();
    void createImageViews();

    // Pipeline
    void createRenderPass();
    void createGraphicsPipeline();

    // Drawing
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    // Helpers
    bool checkValidationLayerSupport();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    bool isDeviceSuitable(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& modes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    static std::vector<char> readFile(const std::string& filename);
    VkShaderModule createShaderModule(const std::vector<char>& code);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);
};

#endif
```

### `VKRenderer::initialize()` and `VKRenderer::cleanup()`

```cpp
void VKRenderer::initialize(SDL_Window* win) {
    window = win;

    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapchain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();

    initialized = true;
    std::cout << "VKRenderer initialized!" << std::endl;
}

void VKRenderer::cleanup() {
    vkDeviceWaitIdle(device);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(device, commandPool, nullptr);

    for (auto framebuffer : swapchainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);

    for (auto imageView : swapchainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroyDevice(device, nullptr);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}
```

### Destruction Order

Destroy in reverse order of creation. If object A was needed to create object B, destroy B before A.

### What You Should See

After compiling the shaders and running the program, you should see a colorful triangle (red, green, blue vertices) on a dark background.

---

**Continue to [Part 2: Working with Data — Vertex Buffers, Uniforms & Textures](VULKAN_TUTORIAL_PART2.md)**
