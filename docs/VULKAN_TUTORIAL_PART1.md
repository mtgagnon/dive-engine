# Vulkan Tutorial for Dive Engine

This tutorial teaches Vulkan from the ground up, using your existing engine code as context. By the end, you'll have a working `VKRenderer` that renders a rotating cube, and understand how to extend it for full 3D actor support.

## Table of Contents

1. [Vulkan vs SDL Renderer: Mental Model Shift](#1-vulkan-vs-sdl-renderer-mental-model-shift)
2. [Vulkan Architecture Overview](#2-vulkan-architecture-overview)
3. [Step 1: Instance Creation](#3-step-1-instance-creation)
4. [Step 2: Surface Creation](#4-step-2-surface-creation)
5. [Step 3: Physical Device Selection](#5-step-3-physical-device-selection)
6. [Step 4: Logical Device & Queues](#6-step-4-logical-device--queues)
7. [Step 5: Swapchain](#7-step-5-swapchain)
8. [Step 6: Image Views](#8-step-6-image-views)
9. [Step 7: Render Pass](#9-step-7-render-pass)
10. [Step 8: Graphics Pipeline & Shaders](#10-step-8-graphics-pipeline--shaders)
11. [Step 9: Framebuffers](#11-step-9-framebuffers)
12. [Step 10: Command Pool & Buffers](#12-step-10-command-pool--buffers)
13. [Step 11: Synchronization](#13-step-11-synchronization)
14. [Step 12: Vertex Buffers & VMA](#14-step-12-vertex-buffers--vma)
15. [Step 13: The Render Loop](#15-step-13-the-render-loop)
16. [Step 14: Uniforms & Push Constants (MVP Matrix)](#16-step-14-uniforms--push-constants-mvp-matrix)
17. [Step 15: Depth Buffer (3D)](#17-step-15-depth-buffer-3d)
18. [Step 16: Rendering a Rotating Cube](#18-step-16-rendering-a-rotating-cube)
19. [Next Steps: 3D Actors](#19-next-steps-3d-actors)

---

## 1. Vulkan vs SDL Renderer: Mental Model Shift

### Your Current SDLRenderer Flow

```cpp
// SDLRenderer (simplified)
SDL_CreateWindow()      // Create window
SDL_CreateRenderer()    // Create renderer (GPU context hidden from you)
SDL_RenderClear()       // Clear screen
SDL_RenderCopy()        // Draw texture
SDL_RenderPresent()     // Show frame
```

SDL hides all GPU details. You just say "draw this texture here" and it happens.

### Vulkan Flow

```cpp
// VKRenderer (simplified)
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

**Key difference:** In SDL, you call "draw" and it draws immediately. In Vulkan, you *record* commands into a buffer, then *submit* that buffer to the GPU.

### Why So Verbose?

Vulkan gives you control over:
- Memory allocation (where textures/buffers live)
- Synchronization (when GPU/CPU wait for each other)
- Pipeline state (shaders, blend modes, depth testing)
- Command recording (batch many draws efficiently)

This verbosity enables:
- Better performance (less driver overhead)
- Predictable behavior (you control everything)
- Cross-platform consistency (same code everywhere)

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

## 3. Step 1: Instance Creation

The instance connects your app to the Vulkan driver and specifies which extensions you need.

### Code

```cpp
// In VKRenderer.h
class VKRenderer {
public:
    static void initialize(SDL_Window* window);
    static void cleanup();
    
private:
    static void createInstance();
    
    inline static VkInstance instance = VK_NULL_HANDLE;
    inline static SDL_Window* windowRef = nullptr;
};
```

```cpp
// In VKRenderer.cpp
#include "VKRenderer.h"
#include "SDL_vulkan.h"
#include <iostream>
#include <vector>
#include <stdexcept>

void VKRenderer::initialize(SDL_Window* window) {
    windowRef = window;
    createInstance();
    std::cout << "Vulkan instance created!" << std::endl;
}

void VKRenderer::createInstance() {
    // App info (optional but good practice)
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Dive Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Dive Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;
    
    // Get extensions SDL needs for Vulkan surface
    unsigned int extensionCount = 0;
    SDL_Vulkan_GetInstanceExtensions(windowRef, &extensionCount, nullptr);
    std::vector<const char*> extensions(extensionCount);
    SDL_Vulkan_GetInstanceExtensions(windowRef, &extensionCount, extensions.data());
    
    // On macOS, add portability extensions for MoltenVK
    #ifdef __APPLE__
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    extensions.push_back("VK_KHR_get_physical_device_properties2");
    #endif
    
    // Create instance
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledLayerCount = 0;  // Validation layers (debug only)
    
    #ifdef __APPLE__
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    #endif
    
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance");
    }
}

void VKRenderer::cleanup() {
    vkDestroyInstance(instance, nullptr);
}
```

### Key Concepts

**`VkApplicationInfo`**: Metadata about your app. Optional but helps drivers optimize.

**Extensions**: Vulkan is modular. The base API is minimal; extensions add features:
- `VK_KHR_surface` - Window surfaces (SDL requests this)
- `VK_KHR_win32_surface` / `VK_KHR_xlib_surface` - Platform-specific surface (SDL requests)
- `VK_KHR_portability_enumeration` - Needed for MoltenVK on macOS

**`sType`**: Every Vulkan struct has a `sType` field. This enables the driver to validate and version structures.

**`nullptr` allocator**: The last parameter to most `vkCreate*` functions is a custom allocator. We pass `nullptr` to use default allocation.

### Compare to SDL

```cpp
// SDL: Implicit instance creation
SDL_Init(SDL_INIT_VIDEO);

// Vulkan: Explicit instance creation
vkCreateInstance(&createInfo, nullptr, &instance);
```

---

## 4. Step 2: Surface Creation

The surface connects Vulkan to your SDL window. This is where SDL helps—it handles platform differences.

### Code

```cpp
// Add to VKRenderer.h
inline static VkSurfaceKHR surface = VK_NULL_HANDLE;
static void createSurface();

// Add to VKRenderer.cpp
void VKRenderer::createSurface() {
    if (!SDL_Vulkan_CreateSurface(windowRef, instance, &surface)) {
        throw std::runtime_error("Failed to create Vulkan surface: " + 
                                 std::string(SDL_GetError()));
    }
}

// Update initialize()
void VKRenderer::initialize(SDL_Window* window) {
    windowRef = window;
    createInstance();
    createSurface();
    std::cout << "Vulkan surface created!" << std::endl;
}

// Update cleanup()
void VKRenderer::cleanup() {
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}
```

### Key Concepts

**Why SDL helps here**: Without SDL, you'd need platform-specific code:

```cpp
// Linux (X11)
VkXlibSurfaceCreateInfoKHR createInfo{};
createInfo.dpy = display;
createInfo.window = window;
vkCreateXlibSurfaceKHR(instance, &createInfo, nullptr, &surface);

// Windows
VkWin32SurfaceCreateInfoKHR createInfo{};
createInfo.hwnd = hwnd;
createInfo.hinstance = hinstance;
vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface);

// macOS (Metal via MoltenVK)
VkMetalSurfaceCreateInfoEXT createInfo{};
createInfo.pLayer = metalLayer;
vkCreateMetalSurfaceEXT(instance, &createInfo, nullptr, &surface);
```

SDL's `SDL_Vulkan_CreateSurface()` handles all this for you.

### Compare to SDL

```cpp
// SDL: Window IS the surface
SDL_Window* window = SDL_CreateWindow(..., SDL_WINDOW_SHOWN);

// Vulkan: Window and surface are separate
SDL_Window* window = SDL_CreateWindow(..., SDL_WINDOW_VULKAN);
SDL_Vulkan_CreateSurface(window, instance, &surface);
```

---

## 5. Step 3: Physical Device Selection

A physical device represents your GPU. You query its capabilities and choose one.

### Code

```cpp
// Add to VKRenderer.h
inline static VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
static void pickPhysicalDevice();
static bool isDeviceSuitable(VkPhysicalDevice device);

// Add to VKRenderer.cpp
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
    
    // Print selected GPU name
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physicalDevice, &props);
    std::cout << "Selected GPU: " << props.deviceName << std::endl;
}

bool VKRenderer::isDeviceSuitable(VkPhysicalDevice device) {
    // For now, accept any device
    // We'll add more checks later (queue families, extensions, swapchain support)
    return true;
}
```

### Key Concepts

**Enumeration pattern**: Vulkan uses a common pattern for listing things:
1. Call with `nullptr` to get count
2. Allocate array
3. Call again to fill array

```cpp
uint32_t count = 0;
vkEnumerateSomething(handle, &count, nullptr);      // Get count
std::vector<Thing> things(count);
vkEnumerateSomething(handle, &count, things.data()); // Fill array
```

**Physical device properties**: You can query GPU info:

```cpp
VkPhysicalDeviceProperties props;
vkGetPhysicalDeviceProperties(device, &props);
// props.deviceName - GPU name
// props.deviceType - VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, etc.
// props.limits - Max texture size, max uniform buffers, etc.
```

**Device suitability**: In a real app, you'd check:
- Does it have a graphics queue?
- Does it support presenting to our surface?
- Does it support required extensions (swapchain)?
- Does it have adequate swapchain capabilities?

We'll add these checks as we go.

---

## 6. Step 4: Logical Device & Queues

The logical device is your interface to the GPU. Queues are where you submit commands.

### Queue Families

GPUs have different types of queues:
- **Graphics queue**: Rendering commands
- **Compute queue**: Compute shaders
- **Transfer queue**: Memory copies
- **Present queue**: Presenting to screen

Often the graphics queue can also present, but not always.

### Code

```cpp
// Add to VKRenderer.h
#include <optional>

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    
    bool isComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

inline static VkDevice device = VK_NULL_HANDLE;
inline static VkQueue graphicsQueue = VK_NULL_HANDLE;
inline static VkQueue presentQueue = VK_NULL_HANDLE;

static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
static void createLogicalDevice();
```

```cpp
// Add to VKRenderer.cpp
#include <set>

QueueFamilyIndices VKRenderer::findQueueFamilies(VkPhysicalDevice dev) {
    QueueFamilyIndices indices;
    
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, queueFamilies.data());
    
    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        // Check for graphics support
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }
        
        // Check for present support
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

void VKRenderer::createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    
    // Create queue create infos (one per unique queue family)
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
    
    // Specify device features we need (none for now)
    VkPhysicalDeviceFeatures deviceFeatures{};
    
    // Required device extensions
    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        #ifdef __APPLE__
        "VK_KHR_portability_subset",
        #endif
    };
    
    // Create the logical device
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    
    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device");
    }
    
    // Get queue handles
    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

// Update isDeviceSuitable()
bool VKRenderer::isDeviceSuitable(VkPhysicalDevice dev) {
    QueueFamilyIndices indices = findQueueFamilies(dev);
    return indices.isComplete();
}

// Update initialize()
void VKRenderer::initialize(SDL_Window* window) {
    windowRef = window;
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    std::cout << "Vulkan device created!" << std::endl;
}

// Update cleanup()
void VKRenderer::cleanup() {
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}
```

### Key Concepts

**Queue families**: Groups of queues with the same capabilities. You request queues from families.

**Queue priority**: 0.0 to 1.0, affects scheduling. Usually just use 1.0.

**Device extensions**: Like instance extensions, but for device features:
- `VK_KHR_swapchain` - Required for presenting to screen

**Device features**: GPU capabilities you want to enable (geometry shaders, multi-viewport, etc.). We leave this empty for now.

### Compare to SDL

```cpp
// SDL: Renderer implicitly creates GPU context
SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

// Vulkan: Explicit device and queue creation
vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
vkGetDeviceQueue(device, queueFamily, 0, &graphicsQueue);
```

---

## 7. Step 5: Swapchain

The swapchain is a queue of images that get presented to the screen. Double or triple buffering.

### Why a Swapchain?

Without buffering:
1. GPU writes to screen buffer
2. Screen reads buffer while GPU writes → tearing

With double buffering:
1. GPU writes to back buffer
2. GPU finishes, swaps buffers
3. Screen reads front buffer (complete image)
4. GPU writes to (now back) buffer

### Code

```cpp
// Add to VKRenderer.h
struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

inline static VkSwapchainKHR swapchain = VK_NULL_HANDLE;
inline static std::vector<VkImage> swapchainImages;
inline static VkFormat swapchainImageFormat;
inline static VkExtent2D swapchainExtent;

static SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device);
static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& modes);
static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
static void createSwapchain();
```

```cpp
// Add to VKRenderer.cpp
#include <algorithm>
#include <limits>

SwapchainSupportDetails VKRenderer::querySwapchainSupport(VkPhysicalDevice dev) {
    SwapchainSupportDetails details;
    
    // Get capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surface, &details.capabilities);
    
    // Get formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &formatCount, details.formats.data());
    }
    
    // Get present modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &presentModeCount, details.presentModes.data());
    }
    
    return details;
}

VkSurfaceFormatKHR VKRenderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
    // Prefer SRGB color space for gamma-correct rendering
    for (const auto& format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }
    return formats[0];  // Fallback to first available
}

VkPresentModeKHR VKRenderer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& modes) {
    // VK_PRESENT_MODE_MAILBOX_KHR - Triple buffering (low latency, no tearing)
    // VK_PRESENT_MODE_FIFO_KHR - VSync (guaranteed available)
    // VK_PRESENT_MODE_IMMEDIATE_KHR - No sync (tearing possible)
    
    for (const auto& mode : modes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return mode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;  // Always available
}

VkExtent2D VKRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    
    // Query actual window size
    int width, height;
    SDL_Vulkan_GetDrawableSize(windowRef, &width, &height);
    
    VkExtent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };
    
    // Clamp to supported range
    actualExtent.width = std::clamp(actualExtent.width,
        capabilities.minImageExtent.width,
        capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height,
        capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height);
    
    return actualExtent;
}

void VKRenderer::createSwapchain() {
    SwapchainSupportDetails support = querySwapchainSupport(physicalDevice);
    
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(support.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(support.presentModes);
    VkExtent2D extent = chooseSwapExtent(support.capabilities);
    
    // Request one more image than minimum (for triple buffering)
    uint32_t imageCount = support.capabilities.minImageCount + 1;
    if (support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount) {
        imageCount = support.capabilities.maxImageCount;
    }
    
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;  // Always 1 unless stereoscopic 3D
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    // Handle queue family sharing
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
    
    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    
    createInfo.preTransform = support.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;  // Don't render pixels obscured by other windows
    createInfo.oldSwapchain = VK_NULL_HANDLE;  // For recreating swapchain (resize)
    
    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swapchain");
    }
    
    // Get swapchain images
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());
    
    swapchainImageFormat = surfaceFormat.format;
    swapchainExtent = extent;
    
    std::cout << "Swapchain created with " << imageCount << " images" << std::endl;
}
```

### Key Concepts

**Surface format**: Pixel format and color space
- `VK_FORMAT_B8G8R8A8_SRGB` - 8 bits per channel, SRGB color space

**Present modes**:
| Mode | Description |
|------|-------------|
| `IMMEDIATE` | No wait, may tear |
| `FIFO` | VSync, wait for vertical blank |
| `FIFO_RELAXED` | VSync, but may tear if late |
| `MAILBOX` | Triple buffer, replace pending image |

**Extent**: Swapchain image resolution (usually matches window size)

**Image sharing**: If graphics and present queues are different families, images need sharing mode.

### Compare to Your Engine's Frame Cycle

```cpp
// SDLRenderer
SDL_RenderClear(renderer);    // Clear back buffer
// ... draw ...
SDL_RenderPresent(renderer);  // Swap buffers (implicit)

// Vulkan
vkAcquireNextImageKHR(...);   // Get index of next swapchain image
// ... record commands to draw to swapchainImages[imageIndex] ...
vkQueuePresentKHR(...);       // Present that image
```

---

## 8. Step 6: Image Views

Images can't be accessed directly in pipelines. You need an `VkImageView` that describes how to interpret the image data.

### Code

```cpp
// Add to VKRenderer.h
inline static std::vector<VkImageView> swapchainImageViews;
static void createImageViews();

// Add to VKRenderer.cpp
void VKRenderer::createImageViews() {
    swapchainImageViews.resize(swapchainImages.size());
    
    for (size_t i = 0; i < swapchainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapchainImageFormat;
        
        // No swizzling (color channel remapping)
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        
        // Use as color target, no mipmaps, single layer
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

// Update cleanup() - destroy in reverse order of creation
void VKRenderer::cleanup() {
    for (auto imageView : swapchainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}
```

### Key Concepts

**View type**: How to interpret the image
- `VK_IMAGE_VIEW_TYPE_2D` - Normal 2D texture
- `VK_IMAGE_VIEW_TYPE_CUBE` - Cubemap
- `VK_IMAGE_VIEW_TYPE_2D_ARRAY` - Array of 2D textures

**Subresource range**: Which parts of the image this view accesses
- `aspectMask` - Color, depth, or stencil
- `baseMipLevel` / `levelCount` - Which mipmap levels
- `baseArrayLayer` / `layerCount` - Which array layers

---

## 9. Step 7: Render Pass

A render pass describes:
- What attachments (color, depth) are used
- How they're loaded/stored
- What subpasses (rendering phases) exist

### Code

```cpp
// Add to VKRenderer.h
inline static VkRenderPass renderPass = VK_NULL_HANDLE;
static void createRenderPass();

// Add to VKRenderer.cpp
void VKRenderer::createRenderPass() {
    // Describe the color attachment (our swapchain image)
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;  // No multisampling
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;  // Clear at start
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;  // Keep results
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;  // Don't care about previous contents
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // Ready for presentation
    
    // Reference to the attachment from a subpass
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;  // Index in attachment array
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    // The subpass
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    
    // Subpass dependency (synchronization)
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;  // Before render pass
    dependency.dstSubpass = 0;  // Our subpass
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    
    // Create render pass
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
| `LOAD` | Keep existing contents |
| `DONT_CARE` | Contents undefined (fastest) |

| storeOp | Description |
|---------|-------------|
| `STORE` | Keep results |
| `DONT_CARE` | Contents undefined after |

**Image layouts**: Images must be in specific layouts for different operations:
- `UNDEFINED` - Don't care (initial state)
- `COLOR_ATTACHMENT_OPTIMAL` - Best for rendering to
- `PRESENT_SRC_KHR` - Ready for presentation

**Subpasses**: Multiple rendering phases within a render pass. Useful for deferred rendering. We just have one.

**Dependencies**: Synchronization between subpasses. `VK_SUBPASS_EXTERNAL` refers to commands before/after the render pass.

### Compare to SDL

```cpp
// SDL: No explicit render pass
SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
SDL_RenderClear(renderer);

// Vulkan: Render pass defines how rendering happens
VkRenderPassBeginInfo beginInfo{};
beginInfo.clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
vkCmdBeginRenderPass(commandBuffer, &beginInfo, ...);
```

---

## 10. Step 8: Graphics Pipeline & Shaders

The graphics pipeline configures everything about how rendering happens:
- Shaders (vertex, fragment)
- Vertex input format
- Rasterization settings
- Blending
- Viewport/scissor

**Vulkan pipelines are immutable** - you can't change settings after creation. Create multiple pipelines for different configurations.

### Shaders

Vulkan uses SPIR-V bytecode, not GLSL directly. You write GLSL, then compile to SPIR-V.

**Vertex shader** (`resources/shaders/triangle.vert`):
```glsl
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform PushConstants {
    mat4 mvp;
} pushConstants;

void main() {
    gl_Position = pushConstants.mvp * vec4(inPosition, 1.0);
    fragColor = inColor;
}
```

**Fragment shader** (`resources/shaders/triangle.frag`):
```glsl
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
```

**Compile to SPIR-V**:
```bash
glslc resources/shaders/triangle.vert -o resources/shaders/triangle.vert.spv
glslc resources/shaders/triangle.frag -o resources/shaders/triangle.frag.spv
```

### Vertex Structure

```cpp
// Add to VKRenderer.h
#include "glm/glm.hpp"
#include <array>

struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
    
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }
    
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attrs{};
        
        // Position
        attrs[0].binding = 0;
        attrs[0].location = 0;  // layout(location = 0)
        attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
        attrs[0].offset = offsetof(Vertex, position);
        
        // Color
        attrs[1].binding = 0;
        attrs[1].location = 1;  // layout(location = 1)
        attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
        attrs[1].offset = offsetof(Vertex, color);
        
        return attrs;
    }
};

struct PushConstants {
    glm::mat4 mvp;
};
```

### Pipeline Code

```cpp
// Add to VKRenderer.h
inline static VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
inline static VkPipeline graphicsPipeline = VK_NULL_HANDLE;

static std::vector<char> readFile(const std::string& filename);
static VkShaderModule createShaderModule(const std::vector<char>& code);
static void createGraphicsPipeline();

// Add to VKRenderer.cpp
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

void VKRenderer::createGraphicsPipeline() {
    // Load shaders
    auto vertShaderCode = readFile("resources/shaders/triangle.vert.spv");
    auto fragShaderCode = readFile("resources/shaders/triangle.frag.spv");
    
    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);
    
    // Shader stages
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";  // Entry point
    
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
    
    // Vertex input
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    
    // Input assembly (how vertices form primitives)
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    
    // Dynamic viewport and scissor (set at draw time)
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
    
    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    
    // Multisampling (disabled)
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    // Color blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;  // No blending for now
    
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    
    // Push constants (for MVP matrix)
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstants);
    
    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout");
    }
    
    // Create the pipeline
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
    
    // Shader modules can be destroyed after pipeline creation
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}
```

### Key Concepts

**Shader modules**: Compiled SPIR-V loaded into Vulkan. Temporary—destroy after pipeline creation.

**Vertex input**: Describes how vertex data is laid out in memory.

**Input assembly**: How vertices form primitives (triangles, lines, points).

**Rasterization**: Fill mode, culling, front face winding.

**Dynamic state**: Some pipeline state can be changed at draw time without recreating the pipeline.

**Push constants**: Small, fast data sent to shaders every draw call. Perfect for MVP matrix.

---

**Continue to [Part 2: Framebuffers, Commands, Render Loop & 3D](VULKAN_TUTORIAL_PART2.md)**
