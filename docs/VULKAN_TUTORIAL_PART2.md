# Vulkan Tutorial for Dive Engine — Part 2: Working with Data

Continuing from [Part 1: Drawing a Triangle](VULKAN_TUTORIAL_PART1.md). This part covers vertex buffers, uniform buffers, push constants, and texture mapping. By the end you'll have a textured, transformed quad with proper GPU memory management via VMA.

## Table of Contents

**Vertex Buffers**
1. [Vertex Input Description](#1-vertex-input-description)
2. [Vertex Buffer Creation with VMA](#2-vertex-buffer-creation-with-vma)
3. [Staging Buffers](#3-staging-buffers)
4. [Index Buffers](#4-index-buffers)

**Uniform Buffers & Push Constants**
5. [Push Constants (MVP Matrix)](#5-push-constants-mvp-matrix)
6. [Descriptor Set Layout and Buffer](#6-descriptor-set-layout-and-buffer)
7. [Descriptor Pool and Sets](#7-descriptor-pool-and-sets)

**Texture Mapping**
8. [Images](#8-images)
9. [Image View and Sampler](#9-image-view-and-sampler)
10. [Combined Image Sampler](#10-combined-image-sampler)
11. [Full Code Checkpoint](#11-full-code-checkpoint)

---

# Vertex Buffers

## 1. Vertex Input Description

In Part 1, triangle vertices were hardcoded in the vertex shader. Now we move them into CPU-side arrays and pass them to the GPU through a vertex buffer — the way a real engine works.

### Updated Vertex Shader (`resources/shaders/triangle.vert`)

```glsl
#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}
```

The `in` keyword means the data comes from a vertex buffer, not from hardcoded arrays. Each `layout(location = N)` corresponds to an attribute we define in C++.

### Fragment Shader (`resources/shaders/triangle.frag`)

```glsl
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
```

Recompile both shaders:

```bash
glslc resources/shaders/triangle.vert -o resources/shaders/triangle.vert.spv
glslc resources/shaders/triangle.frag -o resources/shaders/triangle.frag.spv
```

### Vertex Struct

Add to `VKRenderer.h` (above the class or in the private section):

```cpp
#include "glm/glm.hpp"
#include <array>

struct Vertex {
    glm::vec2 pos;
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

        attrs[0].binding = 0;
        attrs[0].location = 0;
        attrs[0].format = VK_FORMAT_R32G32_SFLOAT;
        attrs[0].offset = offsetof(Vertex, pos);

        attrs[1].binding = 0;
        attrs[1].location = 1;
        attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[1].offset = offsetof(Vertex, color);

        return attrs;
    }
};
```

### Update the Pipeline

In `createGraphicsPipeline()`, replace the empty vertex input with:

```cpp
auto bindingDescription = Vertex::getBindingDescription();
auto attributeDescriptions = Vertex::getAttributeDescriptions();

VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
vertexInputInfo.vertexBindingDescriptionCount = 1;
vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
```

Also update the shader file paths to use `triangle.vert.spv` and `triangle.frag.spv`.

### Key Concepts

**Binding description**: How to read the vertex buffer — stride between entries, per-vertex vs per-instance.

**Attribute descriptions**: How to extract individual fields from each vertex — format, offset, location matching the shader.

**Format mapping**: Vulkan reuses color format enums for vertex attributes:
- `vec2` → `VK_FORMAT_R32G32_SFLOAT`
- `vec3` → `VK_FORMAT_R32G32B32_SFLOAT`
- `vec4` → `VK_FORMAT_R32G32B32A32_SFLOAT`

---

## 2. Vertex Buffer Creation with VMA

### Why VMA?

Without VMA, creating a buffer requires 6 steps: create `VkBuffer`, query memory requirements, find memory type, allocate `VkDeviceMemory`, bind memory to buffer, map and copy. VMA combines this into a single call.

VMA (Vulkan Memory Allocator) is already in your `external/VulkanMemoryAllocator/` directory.

### VMA Setup

In **one** `.cpp` file (VKRenderer.cpp), add before any VMA usage:

```cpp
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
```

Add to `VKRenderer.h` (private section):

```cpp
#include "vk_mem_alloc.h"

VmaAllocator vmaAllocator = VK_NULL_HANDLE;
VkBuffer vertexBuffer = VK_NULL_HANDLE;
VmaAllocation vertexBufferAllocation = VK_NULL_HANDLE;

void createVmaAllocator();
void createVertexBuffer();
```

Add to `VKRenderer.cpp`:

The VMA allocator wraps Vulkan's memory allocation API. It needs handles to the physical device (to query memory properties), logical device (to allocate memory), and instance (for Vulkan version info):

```cpp
void VKRenderer::createVmaAllocator() {
    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice = physicalDevice;
    allocatorInfo.device = device;
    allocatorInfo.instance = instance;
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;

    if (vmaCreateAllocator(&allocatorInfo, &vmaAllocator) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create VMA allocator");
    }
}
```

`createVertexBuffer` starts with our triangle data and a standard `VkBufferCreateInfo`. `VERTEX_BUFFER_BIT` tells Vulkan how the buffer will be used, which helps the driver choose optimal memory placement. `EXCLUSIVE` sharing mode means only one queue family accesses this buffer:

```cpp
void VKRenderer::createVertexBuffer() {
    const std::vector<Vertex> vertices = {
        {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    };

    VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
```

The VMA allocation info controls where the buffer lives in memory. `VMA_MEMORY_USAGE_AUTO` lets VMA choose the best memory type. `HOST_ACCESS_SEQUENTIAL_WRITE_BIT` hints that the CPU will write to this buffer sequentially (not randomly). `MAPPED_BIT` keeps the buffer permanently mapped — `allocationInfo.pMappedData` is a CPU-visible pointer we can `memcpy` into directly, avoiding explicit `vkMapMemory`/`vkUnmapMemory` calls:

```cpp
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                      VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo allocationInfo;

    if (vmaCreateBuffer(vmaAllocator, &bufferInfo, &allocInfo,
                        &vertexBuffer, &vertexBufferAllocation, &allocationInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create vertex buffer");
    }

    memcpy(allocationInfo.pMappedData, vertices.data(), bufferSize);
}
```

### Bind the Vertex Buffer in recordCommandBuffer

After `vkCmdBindPipeline`, add:

```cpp
VkBuffer vertexBuffers[] = { vertexBuffer };
VkDeviceSize offsets[] = { 0 };
vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
```

### Update initialize() and cleanup()

```cpp
// In initialize(), after createLogicalDevice():
createVmaAllocator();

// In initialize(), after createSyncObjects():
createVertexBuffer();

// In cleanup(), before vmaDestroyAllocator:
vmaDestroyBuffer(vmaAllocator, vertexBuffer, vertexBufferAllocation);

// In cleanup(), before vkDestroyDevice:
vmaDestroyAllocator(vmaAllocator);
```

### Key Concepts

**`VMA_MEMORY_USAGE_AUTO`**: VMA picks the best memory type for your usage.

**`VMA_ALLOCATION_CREATE_MAPPED_BIT`**: Keeps the buffer permanently mapped — you can write to `pMappedData` anytime without map/unmap calls.

**Host-visible memory**: CPU can read/write directly. Fine for small, infrequently updated data. For large or frequently accessed data, use a staging buffer (next section).

### Compare to SDLRenderer

```cpp
// SDLRenderer: SDL handles GPU upload automatically
SDL_Texture* tex = IMG_LoadTexture(renderer, "image.png");

// Vulkan: you create the buffer, allocate memory, and copy data
vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &alloc, &info);
memcpy(info.pMappedData, data, size);
```

---

## 3. Staging Buffers

For production use, vertex data should live in device-local memory (fastest for GPU) which the CPU can't write to directly. The staging buffer pattern solves this:

1. Create a **staging buffer** in host-visible memory
2. Copy vertex data into the staging buffer
3. Copy from staging buffer to device-local buffer using a GPU command
4. Destroy the staging buffer

### Helper: Single-Use Command Buffer

Add to `VKRenderer.h`:

```cpp
VkCommandBuffer beginSingleTimeCommands();
void endSingleTimeCommands(VkCommandBuffer commandBuffer);
```

Add to `VKRenderer.cpp`:

`beginSingleTimeCommands` allocates a temporary command buffer for one-shot operations like memory copies. `ONE_TIME_SUBMIT_BIT` tells the driver this buffer will be submitted once and then discarded, enabling potential optimizations:

```cpp
VkCommandBuffer VKRenderer::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}
```

`endSingleTimeCommands` finalizes the buffer, submits it, and waits for the GPU to finish. `vkQueueWaitIdle` is a simple but blocking synchronization — fine during initialization, but in a production engine you'd use fences to avoid stalling the CPU during gameplay:

```cpp
void VKRenderer::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}
```

### Staged Vertex Buffer Creation

Replace `createVertexBuffer()` with a staged version. The staging buffer lives in host-visible memory with `TRANSFER_SRC_BIT` — it's a temporary holding area the CPU can write to:

```cpp
void VKRenderer::createVertexBuffer() {
    const std::vector<Vertex> vertices = {
        {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    };

    VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();

    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;
    VmaAllocationInfo stagingAllocInfo;

    VkBufferCreateInfo stagingBufferInfo{};
    stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferInfo.size = bufferSize;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stagingAllocCreateInfo{};
    stagingAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    stagingAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                   VMA_ALLOCATION_CREATE_MAPPED_BIT;

    vmaCreateBuffer(vmaAllocator, &stagingBufferInfo, &stagingAllocCreateInfo,
                    &stagingBuffer, &stagingAllocation, &stagingAllocInfo);

    memcpy(stagingAllocInfo.pMappedData, vertices.data(), bufferSize);
```

The final vertex buffer lives in device-local memory — the fastest memory type for the GPU, but not directly accessible by the CPU. `TRANSFER_DST_BIT` marks it as a copy destination, and `VERTEX_BUFFER_BIT` marks its intended use. `DEDICATED_MEMORY_BIT` asks VMA to allocate its own `VkDeviceMemory` block for this buffer (best for large, long-lived resources):

```cpp
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    VmaAllocationCreateInfo allocCreateInfo{};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    vmaCreateBuffer(vmaAllocator, &bufferInfo, &allocCreateInfo,
                    &vertexBuffer, &vertexBufferAllocation, nullptr);
```

The GPU copy command transfers data from the staging buffer to the device-local buffer. After the copy finishes, the staging buffer is destroyed — it served its purpose:

```cpp
    VkCommandBuffer cmd = beginSingleTimeCommands();
    VkBufferCopy copyRegion{};
    copyRegion.size = bufferSize;
    vkCmdCopyBuffer(cmd, stagingBuffer, vertexBuffer, 1, &copyRegion);
    endSingleTimeCommands(cmd);

    vmaDestroyBuffer(vmaAllocator, stagingBuffer, stagingAllocation);
}
```

### Key Concepts

**`TRANSFER_SRC_BIT` / `TRANSFER_DST_BIT`**: The staging buffer is a transfer source; the final buffer is a transfer destination.

**`VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT`**: Allocates its own `VkDeviceMemory` block — best for large, long-lived buffers.

**`vkQueueWaitIdle`**: Blocks until the copy completes. Fine for initialization; don't use during rendering.

---

## 4. Index Buffers

Index buffers let you reuse vertices. Instead of duplicating corners shared between triangles, you list vertex indices.

### Code

Add to `VKRenderer.h` (private section):

```cpp
VkBuffer indexBuffer = VK_NULL_HANDLE;
VmaAllocation indexBufferAllocation = VK_NULL_HANDLE;
uint32_t indexCount = 0;

void createIndexBuffer();
```

Update the vertex data to form a quad (4 vertices for a rectangle):

```cpp
void VKRenderer::createVertexBuffer() {
    const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}},
    };
    // ... same staging buffer pattern as above ...
}
```

`createIndexBuffer` follows the same staging pattern. The index data uses `uint16_t` — 16-bit indices support up to 65,535 unique vertices, which is enough for most meshes. Use `uint32_t` (and `VK_INDEX_TYPE_UINT32`) for larger models. The indices `{0, 1, 2, 2, 3, 0}` define two triangles that together form our quad:

```cpp
void VKRenderer::createIndexBuffer() {
    const std::vector<uint16_t> indices = {
        0, 1, 2,
        2, 3, 0,
    };

    indexCount = static_cast<uint32_t>(indices.size());
    VkDeviceSize bufferSize = sizeof(uint16_t) * indices.size();

    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;
    VmaAllocationInfo stagingAllocInfo;

    VkBufferCreateInfo stagingBufferInfo{};
    stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferInfo.size = bufferSize;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stagingAllocCreateInfo{};
    stagingAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    stagingAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                   VMA_ALLOCATION_CREATE_MAPPED_BIT;

    vmaCreateBuffer(vmaAllocator, &stagingBufferInfo, &stagingAllocCreateInfo,
                    &stagingBuffer, &stagingAllocation, &stagingAllocInfo);
    memcpy(stagingAllocInfo.pMappedData, indices.data(), bufferSize);
```

The device-local index buffer uses `INDEX_BUFFER_BIT` instead of `VERTEX_BUFFER_BIT` — otherwise the pattern is identical to the vertex buffer:

```cpp
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    VmaAllocationCreateInfo allocCreateInfo{};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    vmaCreateBuffer(vmaAllocator, &bufferInfo, &allocCreateInfo,
                    &indexBuffer, &indexBufferAllocation, nullptr);

    VkCommandBuffer cmd = beginSingleTimeCommands();
    VkBufferCopy copyRegion{};
    copyRegion.size = bufferSize;
    vkCmdCopyBuffer(cmd, stagingBuffer, indexBuffer, 1, &copyRegion);
    endSingleTimeCommands(cmd);

    vmaDestroyBuffer(vmaAllocator, stagingBuffer, stagingAllocation);
}
```

### Update recordCommandBuffer

After binding the vertex buffer, bind the index buffer and switch from `vkCmdDraw` to `vkCmdDrawIndexed`. The parameters are: index count, instance count, first index, vertex offset, first instance. The vertex offset is useful when multiple meshes share a single vertex buffer:

```cpp
vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
```

Remove the old `vkCmdDraw` call.

---

# Uniform Buffers & Push Constants

## 5. Push Constants (MVP Matrix)

To transform objects in 3D, you need Model-View-Projection matrices sent to the vertex shader. Push constants are the fastest way to send small amounts of data (up to 128 bytes guaranteed).

### When to Use What

| Method | Size Limit | Speed | Use Case |
|--------|-----------|-------|----------|
| Push constants | 128 bytes guaranteed | Fastest | MVP matrix, per-object data |
| Uniform buffers | Much larger | Fast | Scene-wide data, arrays |
| Descriptor sets | Arbitrary | Flexible | Textures, storage buffers |

### Updated Vertex Shader

```glsl
#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform PushConstants {
    mat4 mvp;
} pushConstants;

void main() {
    gl_Position = pushConstants.mvp * vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}
```

### Code

Add to `VKRenderer.h`:

```cpp
struct PushConstants {
    glm::mat4 mvp;
};

float rotationAngle = 0.0f;
```

Update `createGraphicsPipeline()` to add a push constant range to the pipeline layout:

```cpp
VkPushConstantRange pushConstantRange{};
pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
pushConstantRange.offset = 0;
pushConstantRange.size = sizeof(PushConstants);

VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
pipelineLayoutInfo.pushConstantRangeCount = 1;
pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
```

Update `recordCommandBuffer()` to push the MVP matrix:

```cpp
#include "glm/gtc/matrix_transform.hpp"

// After vkCmdBindPipeline, before vkCmdDrawIndexed:
float aspect = static_cast<float>(swapchainExtent.width) /
               static_cast<float>(swapchainExtent.height);

glm::mat4 model = glm::rotate(
    glm::mat4(1.0f), rotationAngle, glm::vec3(0.0f, 0.0f, 1.0f));

glm::mat4 view = glm::lookAt(
    glm::vec3(0.0f, 0.0f, 2.0f),
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
proj[1][1] *= -1;  // Vulkan Y-axis is inverted vs OpenGL/GLM

PushConstants push{};
push.mvp = proj * view * model;

vkCmdPushConstants(commandBuffer, pipelineLayout,
    VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &push);
```

Increment `rotationAngle` in `beginFrame()`:

```cpp
rotationAngle += 0.01f;
```

### Key Concepts

**`proj[1][1] *= -1`**: Vulkan's clip space has Y pointing down (opposite to OpenGL/GLM). This flip corrects it.

**MVP composition**: `projection * view * model * vertex` transforms from model space → world → camera → screen.

### Compare to SDLRenderer

```cpp
// SDLRenderer: camera is a simple 2D offset
SDLRenderer::setCameraPosition(x, y);

// Vulkan: full 3D camera via matrices
glm::mat4 view = glm::lookAt(cameraPos, target, up);
```

---

## 6. Descriptor Set Layout and Buffer

Push constants are limited to ~128 bytes. For larger data (scene lighting, bone matrices, texture bindings), you use **descriptor sets**. A descriptor set is a collection of bindings that connect shader uniforms to GPU resources.

### When to Use Descriptors vs Push Constants

- **Push constants**: Per-object MVP matrix (64 bytes). Updated every draw call.
- **Descriptor sets**: Scene-wide data (lights, camera), textures. Updated less frequently.

### Creating a Descriptor Set Layout

The layout declares what types of resources the shader expects:

```cpp
// Add to VKRenderer.h
VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

void createDescriptorSetLayout();
```

```cpp
// Add to VKRenderer.cpp
void VKRenderer::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout");
    }
}
```

Update the pipeline layout to reference the descriptor set layout:

```cpp
pipelineLayoutInfo.setLayoutCount = 1;
pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
```

### Per-Frame Uniform Buffers

Each frame in flight needs its own uniform buffer (so the CPU can update frame N+1 while the GPU reads frame N):

```cpp
// Add to VKRenderer.h
struct UniformBufferObject {
    glm::mat4 view;
    glm::mat4 proj;
};

std::vector<VkBuffer> uniformBuffers;
std::vector<VmaAllocation> uniformBufferAllocations;
std::vector<void*> uniformBuffersMapped;

void createUniformBuffers();
void updateUniformBuffer(uint32_t currentImage);
```

We create one uniform buffer per frame in flight — since the CPU updates the buffer each frame, we need separate copies so we don't overwrite data the GPU is still reading. `UNIFORM_BUFFER_BIT` marks the buffer for use as a uniform buffer. We keep them persistently mapped (`MAPPED_BIT`) since we update them every frame:

```cpp
void VKRenderer::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBufferAllocations.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                          VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo allocationInfo;
        vmaCreateBuffer(vmaAllocator, &bufferInfo, &allocInfo,
                        &uniformBuffers[i], &uniformBufferAllocations[i], &allocationInfo);
        uniformBuffersMapped[i] = allocationInfo.pMappedData;
    }
}
```

`updateUniformBuffer` writes the current view and projection matrices into the mapped buffer. This is called each frame from `beginFrame()`. The view and projection stay in the uniform buffer (updated once per frame), while the model matrix goes through push constants (updated per draw call) — a common split in engine design:

```cpp
void VKRenderer::updateUniformBuffer(uint32_t frameIndex) {
    float aspect = static_cast<float>(swapchainExtent.width) /
                   static_cast<float>(swapchainExtent.height);

    UniformBufferObject ubo{};
    ubo.view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 2.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    ubo.proj[1][1] *= -1;

    memcpy(uniformBuffersMapped[frameIndex], &ubo, sizeof(ubo));
}
```

---

## 7. Descriptor Pool and Sets

### Creating the Descriptor Pool

```cpp
// Add to VKRenderer.h
VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
std::vector<VkDescriptorSet> descriptorSets;

void createDescriptorPool();
void createDescriptorSets();
```

The descriptor pool pre-allocates memory for descriptor sets — you tell it how many descriptors of each type you'll need. `poolSize` says we need `MAX_FRAMES_IN_FLIGHT` uniform buffer descriptors. `maxSets` limits the total number of descriptor sets that can be allocated from this pool:

```cpp
void VKRenderer::createDescriptorPool() {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool");
    }
}
```

Allocating descriptor sets requires a vector of layouts — one layout per set. We duplicate the same layout for each frame in flight. Descriptor sets are not individually freed; they're all released when the pool is destroyed or reset:

```cpp
void VKRenderer::createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets");
    }
```

After allocation, each descriptor set is empty — we need to write the actual buffer references into them. `VkWriteDescriptorSet` connects a descriptor set binding to a concrete buffer. `dstBinding = 0` matches `binding = 0` in the shader and layout. `dstArrayElement = 0` is the first element in the binding (relevant for arrays of descriptors). Each frame's descriptor set points to that frame's uniform buffer:

```cpp
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
    }
}
```

### Binding Descriptor Sets in recordCommandBuffer

In the command buffer, bind the current frame's descriptor set before drawing. The parameters to `vkCmdBindDescriptorSets` are: the command buffer, bind point (graphics vs compute), pipeline layout, first set index (0), set count (1), the set pointer, and dynamic offset count/values (0 and nullptr since we're not using dynamic offsets):

```cpp
vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
    pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);
```

---

# Texture Mapping

## 8. Images

Loading a texture in Vulkan requires: loading pixels from file, creating a `VkImage`, transitioning its layout, copying pixel data from a staging buffer, and transitioning again for shader access.

### Loading Pixels

We use `stb_image` (add `stb_image.h` to your include path):

```cpp
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
```

### Code

Add to `VKRenderer.h`:

```cpp
VkImage textureImage = VK_NULL_HANDLE;
VmaAllocation textureImageAllocation = VK_NULL_HANDLE;

void createTextureImage();
void transitionImageLayout(VkImage image, VkFormat format,
    VkImageLayout oldLayout, VkImageLayout newLayout);
void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
```

`transitionImageLayout` uses a pipeline barrier to change an image's memory layout. Vulkan images must be in specific layouts for different operations — you can't copy into an image that's in `UNDEFINED` layout or sample from one that's in `TRANSFER_DST_OPTIMAL`. The `VkImageMemoryBarrier` specifies the old and new layouts, and which parts of the image are affected. `srcQueueFamilyIndex` / `dstQueueFamilyIndex` are for transferring ownership between queue families — `VK_QUEUE_FAMILY_IGNORED` means no transfer:

```cpp
void VKRenderer::transitionImageLayout(VkImage image, VkFormat format,
    VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
```

The `srcAccessMask` / `dstAccessMask` and stage flags define what operations must complete before the transition and what operations can proceed after. We handle two transitions:

1. **UNDEFINED → TRANSFER_DST**: Before we can copy pixels into the image. No previous access to wait for (`srcAccessMask = 0`), and transfer writes must wait (`dstAccessMask = TRANSFER_WRITE_BIT`). `TOP_OF_PIPE` means "don't wait for anything" and `TRANSFER` means "the transfer stage must wait."
2. **TRANSFER_DST → SHADER_READ_ONLY**: After the copy, before the fragment shader reads it. Transfer writes must finish before shader reads can begin:

```cpp
    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::runtime_error("Unsupported layout transition");
    }

    vkCmdPipelineBarrier(commandBuffer,
        sourceStage, destinationStage,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    endSingleTimeCommands(commandBuffer);
}
```

`copyBufferToImage` copies pixel data from a staging buffer into a `VkImage`. The `VkBufferImageCopy` region describes the copy — `bufferRowLength` and `bufferImageHeight` of 0 means the pixels are tightly packed (no padding between rows). The image must be in `TRANSFER_DST_OPTIMAL` layout when this command executes:

```cpp
void VKRenderer::copyBufferToImage(VkBuffer buffer, VkImage image,
    uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(commandBuffer, buffer, image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands(commandBuffer);
}
```

Now `createTextureImage` orchestrates the full process. First we load the pixel data using `stb_image`. `STBI_rgb_alpha` forces 4-channel output even if the image file has fewer channels, giving us a consistent `R8G8B8A8` format:

```cpp
void VKRenderer::createTextureImage() {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("resources/images/texture.png",
        &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    if (!pixels) {
        throw std::runtime_error("Failed to load texture image");
    }

    VkDeviceSize imageSize = texWidth * texHeight * 4;
```

The staging buffer pattern is the same as with vertex buffers — load the pixel data into host-visible memory, then we'll copy it to the GPU:

```cpp
    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;
    VmaAllocationInfo stagingAllocInfo;

    VkBufferCreateInfo stagingBufferInfo{};
    stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferInfo.size = imageSize;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stagingAllocCreateInfo{};
    stagingAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    stagingAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                   VMA_ALLOCATION_CREATE_MAPPED_BIT;

    vmaCreateBuffer(vmaAllocator, &stagingBufferInfo, &stagingAllocCreateInfo,
                    &stagingBuffer, &stagingAllocation, &stagingAllocInfo);
    memcpy(stagingAllocInfo.pMappedData, pixels, imageSize);
    stbi_image_free(pixels);
```

The `VkImageCreateInfo` describes the texture's properties. Key fields: `tiling = OPTIMAL` means the driver arranges texels in the most efficient layout for GPU access (as opposed to `LINEAR` which is row-major like CPU memory). `usage` combines `TRANSFER_DST_BIT` (we'll copy into it) and `SAMPLED_BIT` (shaders will sample from it). `format = R8G8B8A8_SRGB` matches what `stb_image` gave us:

```cpp
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(texWidth);
    imageInfo.extent.height = static_cast<uint32_t>(texHeight);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo imageAllocInfo{};
    imageAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    imageAllocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    vmaCreateImage(vmaAllocator, &imageInfo, &imageAllocInfo,
                   &textureImage, &textureImageAllocation, nullptr);
```

Finally, the three-step transfer sequence: transition the image to accept transfers, copy the pixel data from the staging buffer, then transition the image to a shader-readable layout. This is the core pattern for uploading any image data to the GPU:

```cpp
    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(stagingBuffer, textureImage, texWidth, texHeight);
    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vmaDestroyBuffer(vmaAllocator, stagingBuffer, stagingAllocation);
}
```

### Compare to SDLRenderer

```cpp
// SDLRenderer: one call loads, uploads, and creates GPU handle
SDL_Texture* tex = IMG_LoadTexture(renderer, "resources/images/texture.png");

// Vulkan: load pixels, create staging buffer, create VkImage,
//         transition layout, copy data, transition again
```

---

## 9. Image View and Sampler

The texture image needs a `VkImageView` (how to interpret the image data) and a `VkSampler` (how to filter/address the texture).

### Code

Add to `VKRenderer.h`:

```cpp
VkImageView textureImageView = VK_NULL_HANDLE;
VkSampler textureSampler = VK_NULL_HANDLE;

void createTextureImageView();
void createTextureSampler();
```

The image view is similar to the swapchain image views from Part 1 — it tells Vulkan how to interpret the image data:

```cpp
void VKRenderer::createTextureImageView() {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = textureImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &viewInfo, nullptr, &textureImageView) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create texture image view");
    }
}
```

The sampler controls how the GPU reads texels from the texture. `magFilter` and `minFilter` control interpolation when the texture is magnified or minified — `LINEAR` blends neighboring texels for smooth results, while `NEAREST` would give pixel-art-style hard edges.

`addressMode` controls what happens when texture coordinates go outside [0, 1]: `REPEAT` tiles the texture, `CLAMP_TO_EDGE` stretches the edge color, `MIRRORED_REPEAT` mirrors at the boundary.

`anisotropyEnable` and `maxAnisotropy` enable anisotropic filtering, which dramatically improves texture quality viewed at oblique angles (like floors or roads receding into the distance). We query the device's maximum supported level. Note: you must enable `samplerAnisotropy` in the device features struct when creating the logical device:

```cpp
void VKRenderer::createTextureSampler() {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create texture sampler");
    }
}
```

---

## 10. Combined Image Sampler

To use the texture in a shader, you bind it through a descriptor set.

### Update Descriptor Set Layout

Add a second binding for the combined image sampler. `COMBINED_IMAGE_SAMPLER` means the image view and sampler are bound together as a single unit — this matches the `sampler2D` type in GLSL. Binding 0 is our uniform buffer (accessed by the vertex shader), and binding 1 is the texture (accessed by the fragment shader):

```cpp
void VKRenderer::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
        uboLayoutBinding, samplerLayoutBinding
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout");
    }
}
```

### Update Descriptor Pool and Sets

The pool must have enough descriptors for every type we use. Since we added a combined image sampler binding, we need to add a second pool size entry. The pool sizes must match or exceed what our descriptor set layouts require:

```cpp
std::array<VkDescriptorPoolSize, 2> poolSizes{};
poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
poolInfo.pPoolSizes = poolSizes.data();
```

In `createDescriptorSets()`, we now write two descriptors per set. The `VkDescriptorImageInfo` packages the image view, sampler, and current layout together — this is what the `sampler2D` in the shader will actually reference. Each write targets a specific `dstBinding` in the descriptor set:

```cpp
VkDescriptorImageInfo imageInfo{};
imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
imageInfo.imageView = textureImageView;
imageInfo.sampler = textureSampler;

std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
descriptorWrites[0].dstSet = descriptorSets[i];
descriptorWrites[0].dstBinding = 0;
descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
descriptorWrites[0].descriptorCount = 1;
descriptorWrites[0].pBufferInfo = &bufferInfo;

descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
descriptorWrites[1].dstSet = descriptorSets[i];
descriptorWrites[1].dstBinding = 1;
descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
descriptorWrites[1].descriptorCount = 1;
descriptorWrites[1].pImageInfo = &imageInfo;

vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()),
    descriptorWrites.data(), 0, nullptr);
```

### Updated Fragment Shader (for textured rendering)

The `sampler2D` at `binding = 1` corresponds to the combined image sampler we just set up. The `texture()` function samples the image at the given UV coordinates, applying the filtering and address mode settings from the sampler:

```glsl
#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(texSampler, fragTexCoord);
}
```

You'll also need to add texture coordinates to the `Vertex` struct and update the vertex shader to pass them through. See the full checkpoint below.

---

## 11. Full Code Checkpoint

At this point, your `initialize()` should call (in order):

```cpp
createInstance();
setupDebugMessenger();
createSurface();
pickPhysicalDevice();
createLogicalDevice();
createVmaAllocator();
createSwapchain();
createImageViews();
createRenderPass();
createDescriptorSetLayout();
createGraphicsPipeline();
createFramebuffers();
createCommandPool();
createCommandBuffers();
createTextureImage();
createTextureImageView();
createTextureSampler();
createVertexBuffer();
createIndexBuffer();
createUniformBuffers();
createDescriptorPool();
createDescriptorSets();
createSyncObjects();
```

And `cleanup()` destroys in reverse order:

```cpp
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

for (auto framebuffer : swapchainFramebuffers) {
    vkDestroyFramebuffer(device, framebuffer, nullptr);
}

vkDestroyPipeline(device, graphicsPipeline, nullptr);
vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
vkDestroyRenderPass(device, renderPass, nullptr);

for (auto imageView : swapchainImageViews) {
    vkDestroyImageView(device, imageView, nullptr);
}

vkDestroySwapchainKHR(device, swapchain, nullptr);
vmaDestroyAllocator(vmaAllocator);
vkDestroyDevice(device, nullptr);

if (enableValidationLayers) {
    DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
}

vkDestroySurfaceKHR(instance, surface, nullptr);
vkDestroyInstance(instance, nullptr);
```

### What You Should See

A textured, rotating quad with proper GPU memory management. The quad rotates around the Z axis using push constants for the model matrix.

---

**Continue to [Part 3: 3D Rendering — Depth, Models & Resize](VULKAN_TUTORIAL_PART3.md)**
