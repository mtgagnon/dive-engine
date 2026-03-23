# Vulkan Tutorial for Dive Engine — Part 3: 3D Rendering

Continuing from [Part 2: Working with Data](VULKAN_TUTORIAL_PART2.md). This part covers depth buffering, loading 3D models, swapchain recreation (window resize), and rendering a rotating cube. By the end you'll have a proper 3D scene.

## Table of Contents

1. [Depth Buffering](#1-depth-buffering)
2. [Loading Models](#2-loading-models)
3. [Swapchain Recreation](#3-swapchain-recreation)
4. [Rendering a Rotating Cube](#4-rendering-a-rotating-cube)
5. [Full Code Checkpoint](#5-full-code-checkpoint)

---

## 1. Depth Buffering

Without a depth buffer, triangles are drawn in submission order — back faces can appear in front of closer geometry. A depth buffer stores per-pixel depth so closer fragments win.

### What You Need

1. A `VkImage` for depth data
2. A `VkImageView` for the depth image
3. Update the render pass to include a depth attachment
4. Update framebuffers to include the depth image view
5. Add depth-stencil state to the pipeline

### Finding a Depth Format

Add to `VKRenderer.h`:

```cpp
VkImage depthImage = VK_NULL_HANDLE;
VmaAllocation depthImageAllocation = VK_NULL_HANDLE;
VkImageView depthImageView = VK_NULL_HANDLE;

VkFormat findDepthFormat();
void createDepthResources();
```

Add to `VKRenderer.cpp`:

```cpp
VkFormat VKRenderer::findDepthFormat() {
    const std::vector<VkFormat> candidates = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
    };

    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            return format;
        }
    }

    throw std::runtime_error("Failed to find supported depth format");
}
```

### Creating Depth Resources

```cpp
void VKRenderer::createDepthResources() {
    VkFormat depthFormat = findDepthFormat();

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = swapchainExtent.width;
    imageInfo.extent.height = swapchainExtent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = depthFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    if (vmaCreateImage(vmaAllocator, &imageInfo, &allocInfo,
                       &depthImage, &depthImageAllocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create depth image");
    }

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = depthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = depthFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &viewInfo, nullptr, &depthImageView) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create depth image view");
    }
}
```

### Updating the Render Pass

The render pass needs a second attachment for depth:

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

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                               VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass");
    }
}
```

### Updating Framebuffers

```cpp
void VKRenderer::createFramebuffers() {
    swapchainFramebuffers.resize(swapchainImageViews.size());

    for (size_t i = 0; i < swapchainImageViews.size(); i++) {
        std::array<VkImageView, 2> attachments = {
            swapchainImageViews[i],
            depthImageView
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapchainExtent.width;
        framebufferInfo.height = swapchainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapchainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer");
        }
    }
}
```

### Updating the Pipeline

Add depth-stencil state to `createGraphicsPipeline()`:

```cpp
VkPipelineDepthStencilStateCreateInfo depthStencil{};
depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
depthStencil.depthTestEnable = VK_TRUE;
depthStencil.depthWriteEnable = VK_TRUE;
depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
depthStencil.depthBoundsTestEnable = VK_FALSE;
depthStencil.stencilTestEnable = VK_FALSE;

// Add to the pipeline create info:
pipelineInfo.pDepthStencilState = &depthStencil;
```

### Updating recordCommandBuffer for Depth Clear

```cpp
std::array<VkClearValue, 2> clearValues{};
clearValues[0].color = {{{0.1f, 0.1f, 0.15f, 1.0f}}};
clearValues[1].depthStencil = {1.0f, 0};

renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
renderPassInfo.pClearValues = clearValues.data();
```

### Init Order

Call `createDepthResources()` after `createSwapchain()` and `createImageViews()`, but before `createRenderPass()` and `createFramebuffers()`.

### Key Concepts

**Depth format**: `VK_FORMAT_D32_SFLOAT` is 32-bit float depth. `D24_UNORM_S8_UINT` adds an 8-bit stencil channel.

**Depth clear value**: 1.0 means "infinitely far." Any rendered fragment will be closer.

**`depthCompareOp = LESS`**: A fragment passes the depth test if its depth is LESS than the stored value (closer to camera wins).

**Depth storeOp = DONT_CARE**: We don't need the depth data after rendering is complete, so the driver can discard it for better performance.

---

## 2. Loading Models

A game engine needs to load 3D models from files. We'll use [tinyobjloader](https://github.com/syoyo/tinyobjloader), a single-header OBJ loader.

### Setup

Download `tiny_obj_loader.h` and place it in your include path. In one `.cpp` file:

```cpp
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
```

### Upgrading the Vertex Struct for 3D

The vertex struct needs 3D positions and texture coordinates:

```cpp
struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attrs{};

        attrs[0].binding = 0;
        attrs[0].location = 0;
        attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[0].offset = offsetof(Vertex, pos);

        attrs[1].binding = 0;
        attrs[1].location = 1;
        attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[1].offset = offsetof(Vertex, color);

        attrs[2].binding = 0;
        attrs[2].location = 2;
        attrs[2].format = VK_FORMAT_R32G32_SFLOAT;
        attrs[2].offset = offsetof(Vertex, texCoord);

        return attrs;
    }

    bool operator==(const Vertex& other) const {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }
};
```

For vertex deduplication with `std::unordered_map`, add a hash specialization:

```cpp
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
                   (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                   (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}
```

### Loading OBJ Files

Add to `VKRenderer.h`:

```cpp
std::vector<Vertex> vertices;
std::vector<uint32_t> indices;

void loadModel();
```

```cpp
#include <unordered_map>

void VKRenderer::loadModel() {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, "resources/models/model.obj")) {
        throw std::runtime_error("Failed to load model: " + err);
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            if (index.texcoord_index >= 0) {
                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };
            }

            vertex.color = {1.0f, 1.0f, 1.0f};

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVertices[vertex]);
        }
    }
}
```

Call `loadModel()` before `createVertexBuffer()` and `createIndexBuffer()`. Update those functions to use the `vertices` and `indices` member variables instead of local arrays.

### Key Concepts

**OBJ format**: Stores positions, normals, and texture coordinates separately. Each face vertex refers to them by index. `tinyobjloader` triangulates faces automatically.

**Texture coordinate Y-flip**: `1.0f - texcoord.y` because OBJ uses bottom-left origin while Vulkan uses top-left.

**Vertex deduplication**: An `unordered_map` ensures each unique vertex is stored only once, with the index buffer referencing it. This can reduce vertex count dramatically (e.g., 1.5M to 265K for the viking room model).

### Compare to SDLRenderer

```cpp
// SDLRenderer loads 2D images:
SDL_Texture* tex = IMG_LoadTexture(renderer, "resources/images/sprite.png");

// VKRenderer loads 3D models:
loadModel();  // Parse OBJ -> vertices + indices
createVertexBuffer();  // Upload vertices to GPU
createIndexBuffer();   // Upload indices to GPU
```

---

## 3. Swapchain Recreation

When the window is resized or minimized, the swapchain becomes invalid. You need to recreate it along with everything that depends on it.

### Code

Add to `VKRenderer.h`:

```cpp
void recreateSwapchain();
void cleanupSwapchain();
```

```cpp
void VKRenderer::cleanupSwapchain() {
    vkDestroyImageView(device, depthImageView, nullptr);
    vmaDestroyImage(vmaAllocator, depthImage, depthImageAllocation);

    for (auto framebuffer : swapchainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    for (auto imageView : swapchainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapchain, nullptr);
}

void VKRenderer::recreateSwapchain() {
    int width = 0, height = 0;
    SDL_Vulkan_GetDrawableSize(window, &width, &height);
    while (width == 0 || height == 0) {
        SDL_Vulkan_GetDrawableSize(window, &width, &height);
        SDL_WaitEvent(nullptr);
    }

    vkDeviceWaitIdle(device);

    cleanupSwapchain();

    createSwapchain();
    createImageViews();
    createDepthResources();
    createFramebuffers();
}
```

### Triggering Recreation

Update `beginFrame()` and `endFrame()` to handle `VK_ERROR_OUT_OF_DATE_KHR`:

```cpp
void VKRenderer::beginFrame() {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX,
        imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &currentImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    }

    vkResetFences(device, 1, &inFlightFences[currentFrame]);
    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    recordCommandBuffer(commandBuffers[currentFrame], currentImageIndex);
}

void VKRenderer::endFrame() {
    // ... submit and present as before ...

    VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreateSwapchain();
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
```

### Key Concepts

**What needs recreation**: Swapchain, image views, depth resources, framebuffers.

**What doesn't**: Pipeline (uses dynamic viewport/scissor), command pool/buffers, sync objects, vertex/index buffers, textures, render pass.

**`vkDeviceWaitIdle`**: Block until the GPU finishes all work. Required before destroying resources the GPU might still be using.

**Minimized windows**: `SDL_Vulkan_GetDrawableSize` returns 0x0 when minimized. We loop and wait for events until the window is restored.

---

## 4. Rendering a Rotating Cube

Now upgrade from a 2D shape to a 3D cube. This requires 8 vertices, 36 indices (12 triangles), and rotation around multiple axes.

### Cube Vertex Data

If you're not loading an OBJ model, use this hardcoded cube:

```cpp
const std::vector<Vertex> vertices = {
    // Front face
    {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
    // Back face
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}, {1.0f, 1.0f}},
};

const std::vector<uint32_t> indices = {
    0, 1, 2,  2, 3, 0,   // Front
    1, 5, 6,  6, 2, 1,   // Right
    5, 4, 7,  7, 6, 5,   // Back
    4, 0, 3,  3, 7, 4,   // Left
    3, 2, 6,  6, 7, 3,   // Top
    4, 5, 1,  1, 0, 4,   // Bottom
};
```

### Cube Rotation

Update the model matrix in `recordCommandBuffer()` to rotate around multiple axes:

```cpp
glm::mat4 model = glm::mat4(1.0f);
model = glm::rotate(model, rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
model = glm::rotate(model, rotationAngle * 0.7f, glm::vec3(1.0f, 0.0f, 0.0f));

glm::mat4 view = glm::lookAt(
    glm::vec3(0.0f, 0.0f, 3.0f),
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));
```

### Updated Vertex Shader for 3D

```glsl
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

layout(push_constant) uniform PushConstants {
    mat4 mvp;
} pushConstants;

void main() {
    gl_Position = pushConstants.mvp * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}
```

### Pipeline Adjustments for 3D

Update `frontFace` in the rasterizer. With GLM and the Vulkan Y-flip, counter-clockwise is typically correct:

```cpp
rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
```

### Key Concepts

**Index buffers save memory**: A cube has 8 unique vertices but 36 index entries (6 faces x 2 triangles x 3 vertices).

**`VK_INDEX_TYPE_UINT32`**: Use 32-bit indices for meshes with more than 65535 vertices.

**Back-face culling**: `VK_CULL_MODE_BACK_BIT` discards triangles facing away from the camera. Make sure your winding order matches `frontFace`.

---

## 5. Full Code Checkpoint

### Init Order

```cpp
void VKRenderer::initialize(SDL_Window* win) {
    window = win;

    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createVmaAllocator();
    createSwapchain();
    createImageViews();
    createDepthResources();
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffers();
    createTextureImage();
    createTextureImageView();
    createTextureSampler();
    loadModel();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createSyncObjects();

    initialized = true;
}
```

### Cleanup

```cpp
void VKRenderer::cleanup() {
    vkDeviceWaitIdle(device);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    vkDestroyDescriptorPool(device, descriptorPool, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vmaDestroyBuffer(vmaAllocator, uniformBuffers[i], uniformBufferAllocations[i]);
    }

    vmaDestroyBuffer(vmaAllocator, indexBuffer, indexBufferAllocation);
    vmaDestroyBuffer(vmaAllocator, vertexBuffer, vertexBufferAllocation);

    vkDestroySampler(device, textureSampler, nullptr);
    vkDestroyImageView(device, textureImageView, nullptr);
    vmaDestroyImage(vmaAllocator, textureImage, textureImageAllocation);

    vkDestroyCommandPool(device, commandPool, nullptr);

    cleanupSwapchain();

    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);

    vmaDestroyAllocator(vmaAllocator);
    vkDestroyDevice(device, nullptr);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}
```

### Per-Frame Flow

```
beginFrame():
  1. Wait for fence
  2. Acquire swap chain image (handle OUT_OF_DATE)
  3. Reset fence
  4. Update uniform buffer
  5. Reset and record command buffer
     - Begin render pass (clear color + depth)
     - Set viewport/scissor
     - Bind pipeline
     - Bind vertex/index buffers
     - Bind descriptor set
     - Push constants (model MVP)
     - Draw indexed
     - End render pass

endFrame():
  6. Submit command buffer (wait imageAvailable, signal renderFinished)
  7. Present (wait renderFinished, handle OUT_OF_DATE/SUBOPTIMAL)
  8. Advance currentFrame
```

### What You Should See

A textured 3D cube (or loaded model) rotating on screen, with proper depth testing so back faces are hidden, and the window can be resized without crashing.

---

**Continue to [Part 4: Engine Integration & Future Work](VULKAN_TUTORIAL_PART4.md)**
