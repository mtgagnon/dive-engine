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
3. Add a depth attachment to `VkRenderingInfo` in `recordCommandBuffer`
4. Update `VkPipelineRenderingCreateInfo` to include the depth format
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

The depth image is similar to a texture image, but it uses a depth format and is only used as an attachment (never sampled by shaders). The image must match the swapchain extent since there's one depth value per pixel. `DEPTH_STENCIL_ATTACHMENT_BIT` tells Vulkan this image will be used as a depth/stencil attachment during rendering:

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
```

The image view for the depth image uses `VK_IMAGE_ASPECT_DEPTH_BIT` instead of `COLOR_BIT`. If the format includes a stencil component (like `D24_UNORM_S8_UINT`), you'd need a separate view with `STENCIL_BIT` to access the stencil data — but we don't need stencil for basic depth testing:

```cpp
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

### Adding Depth to Dynamic Rendering

With dynamic rendering, there's no render pass or framebuffer to update. Instead, we add a depth attachment directly to the `VkRenderingInfo` struct in `recordCommandBuffer`. This is one of the biggest advantages of dynamic rendering — adding or changing attachments is just adding a struct field, not rearchitecting render pass objects and framebuffers.

#### Transitioning the Depth Image Layout

Just like the swapchain color image, the depth image needs to be in the correct layout before rendering. We reuse the `transitionImageLayout` helper from Part 1, but there's one difference: depth images require `VK_IMAGE_ASPECT_DEPTH_BIT` in the barrier's `aspectMask` instead of `VK_IMAGE_ASPECT_COLOR_BIT`.

The simplest approach is to add an optional `aspectMask` parameter to the helper, defaulting to `VK_IMAGE_ASPECT_COLOR_BIT`:

```cpp
void VKRenderer::transitionImageLayout(VkCommandBuffer cmd, VkImage image,
    VkImageLayout oldLayout, VkImageLayout newLayout,
    VkAccessFlags2 srcAccess, VkAccessFlags2 dstAccess,
    VkPipelineStageFlags2 srcStage, VkPipelineStageFlags2 dstStage,
    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT)
{
    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    // ... same as Part 1 ...
    barrier.subresourceRange.aspectMask = aspectMask;  // Now uses the parameter
    // ... rest unchanged ...
}
```

Add the depth transition in `recordCommandBuffer`, after the color image transition but before `vkCmdBeginRendering`. We transition from `UNDEFINED` (we don't care about previous depth data — we'll clear it) to `DEPTH_ATTACHMENT_OPTIMAL` (the layout the GPU expects for depth testing).

The stage flags specify *when* the transition happens in the GPU pipeline. `EARLY_FRAGMENT_TESTS_BIT` is where depth testing occurs — the GPU reads and writes the depth buffer during this stage. We use the same stage for both source and destination because the depth image isn't used by any earlier stage:

```cpp
// In recordCommandBuffer, after the color image transition:
transitionImageLayout(commandBuffer, depthImage,
    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
    0,                                               // srcAccess: no prior access to wait for
    VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,  // dstAccess: depth writes during fragment tests
    VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,    // srcStage: earliest point depth is used
    VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,    // dstStage: depth testing reads/writes here
    VK_IMAGE_ASPECT_DEPTH_BIT);                      // aspectMask: this is a depth image
```

#### Setting Up the Depth Attachment

Now set up the depth attachment info alongside the color attachment. `VkRenderingAttachmentInfo` describes how to use this image during rendering — the same struct type we used for the color attachment, but configured for depth:

```cpp
VkRenderingAttachmentInfo depthAttachment{};
depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
depthAttachment.imageView = depthImageView;
depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
depthAttachment.clearValue.depthStencil = {1.0f, 0};
```

Breaking down the fields:

- **`imageView`**: The depth image view we created in `createDepthResources()`. Unlike color attachments (which cycle through swapchain images), the depth image view is the same every frame since depth data is never presented.
- **`imageLayout`**: `DEPTH_ATTACHMENT_OPTIMAL` tells the driver to use the optimal memory layout for depth read/write operations.
- **`loadOp = CLEAR`**: Fills the depth buffer with `clearValue` at the start of rendering. Without this, you'd get depth data from the previous frame causing random fragments to be discarded.
- **`storeOp = DONT_CARE`**: We don't need the depth data after rendering completes — it was only needed during the current frame's fragment tests. The driver can discard it, potentially saving bandwidth on tile-based GPUs (mobile, Apple M-series).
- **`clearValue.depthStencil = {1.0f, 0}`**: 1.0 means "infinitely far away." Since our depth test uses `VK_COMPARE_OP_LESS`, any rendered fragment (which will have depth < 1.0) will pass the test against the cleared value. The second value (0) is the stencil clear value, unused here.

#### Wiring Depth into VkRenderingInfo

Add `pDepthAttachment` to the `VkRenderingInfo`. Unlike `pColorAttachments` (which is an array — you could have multiple color attachments for MRT/deferred rendering), `pDepthAttachment` is a single pointer because you can only have one depth attachment:

```cpp
VkRenderingInfo renderInfo{};
renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
renderInfo.renderArea.offset = {0, 0};
renderInfo.renderArea.extent = swapchainExtent;
renderInfo.layerCount = 1;
renderInfo.colorAttachmentCount = 1;
renderInfo.pColorAttachments = &colorAttachment;
renderInfo.pDepthAttachment = &depthAttachment;    // NEW: enables depth testing

vkCmdBeginRendering(commandBuffer, &renderInfo);
```

That's it — no render pass to modify, no framebuffer to rebuild. Adding depth was just two extra structs and a layout transition.

### Updating the Pipeline

Add depth-stencil state to `createGraphicsPipeline()`. `depthTestEnable` turns on depth comparisons — without this, fragments are drawn in submission order regardless of distance. `depthWriteEnable` lets passing fragments update the depth buffer. `VK_COMPARE_OP_LESS` means a fragment passes only if its depth is less than the stored value (closer to camera wins):

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

Also update the `VkPipelineRenderingCreateInfo` (from Part 1) to include the depth format. In Part 1 we only specified `colorAttachmentCount` and `pColorAttachmentFormats`. Now we add `depthAttachmentFormat` so the pipeline knows it will receive a depth attachment during rendering. If this format doesn't match the actual depth image format used in `VkRenderingAttachmentInfo`, validation layers will flag an error:

```cpp
VkFormat depthFormat = findDepthFormat();

VkPipelineRenderingCreateInfo renderingInfo{};
renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
renderingInfo.colorAttachmentCount = 1;
renderingInfo.pColorAttachmentFormats = &swapchainImageFormat;
renderingInfo.depthAttachmentFormat = depthFormat;  // NEW: must match depth image format

pipelineInfo.pNext = &renderingInfo;
pipelineInfo.renderPass = VK_NULL_HANDLE;
```

This is the dynamic rendering equivalent of adding a depth attachment description to a render pass. The key difference: with render passes, changing the depth format would require destroying and recreating the render pass *and* all pipelines using it. With dynamic rendering, you'd only need to recreate the pipeline.

### Init Order

Call `createDepthResources()` after `createSwapchain()` and `createImageViews()`, but before `createGraphicsPipeline()` (since the pipeline now needs the depth format).

### Key Concepts

**Depth format**: `VK_FORMAT_D32_SFLOAT` is 32-bit float depth. `D24_UNORM_S8_UINT` adds an 8-bit stencil channel.

**Depth clear value**: 1.0 means "infinitely far." Any rendered fragment will be closer.

**`depthCompareOp = LESS`**: A fragment passes the depth test if its depth is LESS than the stored value (closer to camera wins).

**Depth storeOp = DONT_CARE**: We don't need the depth data after rendering is complete, so the driver can discard it for better performance.

**Dynamic rendering depth**: Instead of embedding depth attachment info in a render pass, we specify it inline in `VkRenderingInfo::pDepthAttachment`. The depth image layout transition is handled manually via `transitionImageLayout`, just like the color image.

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

The vertex struct upgrades from `vec2` to `vec3` for positions and adds a `texCoord` field. The binding description stays the same (one binding at stride `sizeof(Vertex)`), but we now have 3 attribute descriptions. Each attribute's `location` matches the `layout(location = N)` in the vertex shader, and the `format` tells Vulkan how to interpret the raw bytes — `R32G32B32_SFLOAT` for a 3-component float vector, `R32G32_SFLOAT` for a 2-component one:

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

For vertex deduplication with `std::unordered_map`, we need a hash specialization. GLM provides hash functions for its vector types in the experimental `gtx/hash.hpp` header. The hash combines all three fields using XOR and bit shifts — a standard technique for combining hashes:

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

`tinyobj::LoadObj` parses the OBJ file into three structures: `attrib` contains the raw vertex data (positions, normals, texture coordinates as flat arrays), `shapes` contains the mesh topology (which vertices form each face), and `materials` contains material definitions (unused here). The function returns `false` on failure and populates `err` with details:

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
```

OBJ files store positions, normals, and texture coordinates in separate arrays, with faces referencing them by index. A single face vertex might use position #5, texcoord #12, and normal #3 — different combinations create unique vertices.

We use an `unordered_map` to deduplicate: if a vertex with the same position, color, and texcoord was already seen, we reuse its index. The Y-flip on texture coordinates (`1.0f - y`) converts from OBJ's bottom-left origin to Vulkan's top-left origin. This deduplication can dramatically reduce vertex count (e.g., the viking room model goes from ~1.5M face-vertices to ~265K unique vertices):

```cpp
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

`cleanupSwapchain` destroys everything that depends on the swapchain's size or images. Compare what we need to clean up with and without dynamic rendering:

| Without Dynamic Rendering | With Dynamic Rendering |
|---|---|
| Depth image + view | Depth image + view |
| Framebuffers (one per swapchain image) | *None — no framebuffers exist* |
| Image views | Image views |
| Swapchain | Swapchain |

With render passes, framebuffers were the "glue" between the render pass and the actual images. Each framebuffer referenced specific image views and had to match the render pass attachment count. Dynamic rendering eliminates this entire layer — attachments are specified inline at draw time.

The destroy order matters: dependents must be destroyed before the things they depend on. Depth resources reference the device allocator. Image views reference swapchain images. The swapchain itself goes last:

```cpp
void VKRenderer::cleanupSwapchain() {
    vkDestroyImageView(device, depthImageView, nullptr);
    vmaDestroyImage(vmaAllocator, depthImage, depthImageAllocation);

    for (auto imageView : swapchainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapchain, nullptr);
}
```

`recreateSwapchain` handles the full rebuild. The `while` loop handles minimized windows — when minimized, `SDL_Vulkan_GetDrawableSize` returns 0x0, and we can't create a swapchain with zero extent. We wait for SDL events (which include the restore event) until the window has a valid size again.

`vkDeviceWaitIdle` blocks until the GPU finishes all outstanding work — we can't destroy resources the GPU might still be using. Then we tear down the old swapchain-dependent objects and recreate them:

```cpp
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
}
```

Notice how short this is compared to a traditional render pass setup — with render passes you'd also need `createFramebuffers()` here (one framebuffer per swapchain image, each referencing specific image views). With dynamic rendering, the attachments are specified inline each frame in `recordCommandBuffer`, so there's nothing to pre-create.

### Triggering Recreation

Update `beginFrame()` and `endFrame()` to handle `VK_ERROR_OUT_OF_DATE_KHR`. This error means the swapchain is no longer compatible with the surface — typically because the window was resized. When `vkAcquireNextImageKHR` returns this error, the swapchain can't provide images, so we must recreate it immediately and bail out of the current frame.

Note the fence reset is moved *after* the acquire check — if we reset the fence and then return early for recreation, we'd lose track of that fence's state:

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
```

In `endFrame()`, we check after presentation. `VK_SUBOPTIMAL_KHR` means the swapchain still works but no longer matches the surface properties optimally (e.g., the window was resized but the old swapchain images still display). We treat both `OUT_OF_DATE` and `SUBOPTIMAL` as triggers for recreation:

```cpp
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

**What needs recreation**: Swapchain, image views, depth resources.

**What doesn't**: Pipeline (uses dynamic viewport/scissor), command pool/buffers, sync objects, vertex/index buffers, textures. With dynamic rendering there are no framebuffers or render passes to worry about during recreation.

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
    createDescriptorSetLayout();
    createGraphicsPipeline();
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

Notice `createRenderPass()` and `createFramebuffers()` are gone — dynamic rendering eliminates both of these setup steps.

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

    vmaDestroyAllocator(vmaAllocator);
    vkDestroyDevice(device, nullptr);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}
```

Notice `vkDestroyRenderPass` is gone — with dynamic rendering, there's no render pass object to clean up.

### Per-Frame Flow

```
beginFrame():
  1. Wait for fence
  2. Acquire swap chain image (handle OUT_OF_DATE)
  3. Reset fence
  4. Update uniform buffer
  5. Reset and record command buffer
     - Transition color image: UNDEFINED → COLOR_ATTACHMENT_OPTIMAL
     - Transition depth image: UNDEFINED → DEPTH_ATTACHMENT_OPTIMAL
     - Begin rendering (clear color + depth)
     - Set viewport/scissor
     - Bind pipeline
     - Bind vertex/index buffers
     - Bind descriptor set
     - Push constants (model MVP)
     - Draw indexed
     - End rendering
     - Transition color image: COLOR_ATTACHMENT_OPTIMAL → PRESENT_SRC_KHR

endFrame():
  6. Submit command buffer (wait imageAvailable, signal renderFinished)
  7. Present (wait renderFinished, handle OUT_OF_DATE/SUBOPTIMAL)
  8. Advance currentFrame
```

### What You Should See

A textured 3D cube (or loaded model) rotating on screen, with proper depth testing so back faces are hidden, and the window can be resized without crashing.

---

**Continue to [Part 4: Engine Integration & Future Work](VULKAN_TUTORIAL_PART4.md)**
