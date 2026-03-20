# Vulkan Tutorial for Dive Engine — Part 2

Continuing from [Part 1](VULKAN_TUTORIAL.md). This covers framebuffers, command buffers, synchronization, the render loop, depth buffering, and rendering a rotating cube.

---

## 11. Step 9: Framebuffers

A framebuffer connects your render pass to actual images. Each swapchain image needs its own framebuffer.

Think of it this way:
- **Render pass** describes *what* attachments exist (color, depth) and *how* they're used
- **Framebuffer** says *which specific images* fill those attachment slots

### Code

```cpp
// Add to VKRenderer.h
inline static std::vector<VkFramebuffer> swapchainFramebuffers;
static void createFramebuffers();

// Add to VKRenderer.cpp
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

**One framebuffer per swapchain image**: If you have 3 swapchain images (triple buffering), you create 3 framebuffers.

**Attachment order**: The `pAttachments` array must match the order of attachments in the render pass. We only have one (color), so it's simple. When you add a depth buffer later, you'll pass two attachments.

**Framebuffer dimensions**: Must match the swapchain extent. If the window resizes, you recreate the swapchain AND the framebuffers.

### Compare to SDL

SDL has no framebuffer concept—it just renders to "the screen." In Vulkan, you explicitly choose which image to render into.

---

## 12. Step 10: Command Pool & Buffers

Commands in Vulkan aren't executed immediately. You record them into a **command buffer**, then submit that buffer to a queue.

### Command Pool

A command pool manages memory for command buffers. You need one per queue family.

### Command Buffers

A command buffer stores a sequence of GPU commands. You "record" into it, then "submit" it.

### Code

```cpp
// Add to VKRenderer.h
static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

inline static VkCommandPool commandPool = VK_NULL_HANDLE;
inline static std::vector<VkCommandBuffer> commandBuffers;

static void createCommandPool();
static void createCommandBuffers();

// Add to VKRenderer.cpp
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

### Key Concepts

**`MAX_FRAMES_IN_FLIGHT`**: How many frames the CPU can prepare ahead of the GPU. Usually 2. This means we have 2 command buffers and rotate between them.

**`RESET_COMMAND_BUFFER_BIT`**: Allows individual command buffers to be reset and re-recorded. Without this, you'd have to reset the entire pool.

**Primary vs Secondary**: Primary buffers are submitted directly to queues. Secondary buffers are called from primary buffers (useful for multi-threaded recording).

**Command buffers are not freed**: They're allocated from the pool and returned when the pool is destroyed. You can also explicitly free them with `vkFreeCommandBuffers`.

### Compare to SDL

```
SDL: "Draw this now"
  SDL_RenderCopy(renderer, texture, &src, &dst);

Vulkan: "Record this command, I'll submit it later"
  vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
```

---

## 13. Step 11: Synchronization

This is the trickiest part of Vulkan. You must manually synchronize:
- **CPU ↔ GPU**: CPU shouldn't overwrite a command buffer the GPU is still using
- **GPU ↔ GPU**: Don't present an image before rendering finishes

### Three Sync Primitives

| Primitive | Scope | Use Case |
|-----------|-------|----------|
| **Fence** | CPU ↔ GPU | CPU waits for GPU to finish a frame |
| **Semaphore** | GPU ↔ GPU | Signal between queue operations (acquire → render → present) |
| **Barrier** | Within command buffer | Memory/layout transitions (covered later) |

### The Frame Sync Flow

```
Frame N:

1. CPU: Wait for fence[N] (ensures GPU finished frame N-2)
2. CPU: Acquire next swapchain image (signals imageAvailable semaphore)
3. CPU: Record commands into commandBuffer[N]
4. CPU: Submit commandBuffer[N] to graphics queue
         - Wait on: imageAvailable semaphore (don't render until image acquired)
         - Signal: renderFinished semaphore (rendering done)
         - Signal: fence[N] (CPU can track completion)
5. CPU: Present the image
         - Wait on: renderFinished semaphore (don't present until rendering done)
```

### Code

```cpp
// Add to VKRenderer.h
inline static std::vector<VkSemaphore> imageAvailableSemaphores;
inline static std::vector<VkSemaphore> renderFinishedSemaphores;
inline static std::vector<VkFence> inFlightFences;
inline static uint32_t currentFrame = 0;

static void createSyncObjects();

// Add to VKRenderer.cpp
void VKRenderer::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // Start signaled so first frame doesn't deadlock
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create sync objects");
        }
    }
}
```

### Key Concepts

**`VK_FENCE_CREATE_SIGNALED_BIT`**: Fences start signaled. Without this, the first frame would wait forever because no previous frame signaled the fence.

**Why 2 of everything**: With `MAX_FRAMES_IN_FLIGHT = 2`, the CPU can prepare frame N+1 while the GPU works on frame N. Each frame has its own command buffer, semaphores, and fence.

**Semaphores vs Fences**:
- Semaphores: GPU-to-GPU sync. The CPU never waits on them.
- Fences: GPU-to-CPU sync. The CPU calls `vkWaitForFences` to block until the GPU signals.

### Compare to SDL

```
SDL: No sync needed — SDL_RenderPresent handles everything
  SDL_RenderPresent(renderer);  // Blocks if vsync, swaps buffers

Vulkan: You manage all synchronization
  vkWaitForFences(...)          // Wait for previous frame
  vkAcquireNextImageKHR(...)    // Get image (semaphore)
  vkQueueSubmit(...)            // Submit (semaphore + fence)
  vkQueuePresentKHR(...)        // Present (semaphore)
```

---

## 14. Step 12: Vertex Buffers & VMA

You need GPU-accessible memory to store vertex data. This is where VMA helps.

### Raw Vulkan Memory (what VMA replaces)

Without VMA, allocating a buffer requires:
1. Create `VkBuffer`
2. Query memory requirements
3. Find a suitable memory type
4. Allocate `VkDeviceMemory`
5. Bind memory to buffer
6. Map memory, copy data, unmap

VMA combines steps 2-6 into one call.

### Code

```cpp
// Add to VKRenderer.h
inline static VkBuffer vertexBuffer = VK_NULL_HANDLE;
inline static VmaAllocation vertexBufferAllocation = VK_NULL_HANDLE;

static void createVertexBuffer();

// In ONE .cpp file (VKRenderer.cpp), before any VMA usage:
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

// You also need a VmaAllocator
inline static VmaAllocator allocator = VK_NULL_HANDLE;
static void createAllocator();
```

```cpp
// Add to VKRenderer.cpp
void VKRenderer::createAllocator() {
    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice = physicalDevice;
    allocatorInfo.device = device;
    allocatorInfo.instance = instance;
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
    
    if (vmaCreateAllocator(&allocatorInfo, &allocator) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create VMA allocator");
    }
}

void VKRenderer::createVertexBuffer() {
    // Define triangle vertices
    std::vector<Vertex> vertices = {
        // position              color
        {{0.0f, -0.5f, 0.0f},  {1.0f, 0.0f, 0.0f}},  // Top - Red
        {{0.5f,  0.5f, 0.0f},  {0.0f, 1.0f, 0.0f}},  // Bottom right - Green
        {{-0.5f, 0.5f, 0.0f},  {0.0f, 0.0f, 1.0f}},  // Bottom left - Blue
    };
    
    VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();
    
    // Buffer create info
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    // VMA allocation info
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                      VMA_ALLOCATION_CREATE_MAPPED_BIT;
    
    VmaAllocationInfo allocationInfo;
    
    if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo,
                        &vertexBuffer, &vertexBufferAllocation, &allocationInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create vertex buffer");
    }
    
    // Copy vertex data (buffer is already mapped thanks to VMA_ALLOCATION_CREATE_MAPPED_BIT)
    memcpy(allocationInfo.pMappedData, vertices.data(), bufferSize);
}
```

### Key Concepts

**`VMA_IMPLEMENTATION`**: VMA is header-only. Define this in exactly ONE .cpp file.

**Memory types**: GPUs have different memory types:
- `DEVICE_LOCAL` - Fast GPU memory, CPU can't access directly
- `HOST_VISIBLE` - CPU can read/write, slower for GPU
- `HOST_COHERENT` - No need to flush after CPU writes

For a simple triangle, host-visible mapped memory is fine. For production, you'd use a staging buffer (host-visible) to copy into device-local memory.

**VMA_MEMORY_USAGE_AUTO**: VMA picks the best memory type for you.

**Mapped memory**: `VMA_ALLOCATION_CREATE_MAPPED_BIT` keeps the buffer mapped permanently. You can write to `allocationInfo.pMappedData` anytime without map/unmap calls.

### Compare to SDL

```
SDL: Load image, SDL handles GPU upload
  SDL_Texture* tex = IMG_LoadTexture(renderer, "image.png");

Vulkan: You manage buffer creation, memory allocation, and data upload
  vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation, &allocInfo);
  memcpy(allocationInfo.pMappedData, data, size);
```

---

## 15. Step 13: The Render Loop

Now we put everything together. The render loop has two phases:
1. **beginFrame**: Acquire image, begin command buffer, begin render pass
2. **endFrame**: End render pass, submit commands, present

### Code

```cpp
// Add to VKRenderer.h
inline static uint32_t currentImageIndex = 0;

static void beginFrame();
static void endFrame();
```

```cpp
// Add to VKRenderer.cpp
void VKRenderer::beginFrame() {
    // 1. Wait for the previous frame using this slot to finish
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    
    // 2. Acquire next swapchain image
    VkResult result = vkAcquireNextImageKHR(
        device, swapchain, UINT64_MAX,
        imageAvailableSemaphores[currentFrame],  // Signal when image is available
        VK_NULL_HANDLE,
        &currentImageIndex
    );
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    }
    
    // 3. Reset fence AFTER we know we'll submit work
    vkResetFences(device, 1, &inFlightFences[currentFrame]);
    
    // 4. Reset and begin command buffer
    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    
    if (vkBeginCommandBuffer(commandBuffers[currentFrame], &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer");
    }
    
    // 5. Begin render pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapchainFramebuffers[currentImageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchainExtent;
    
    VkClearValue clearColor = {{{0.1f, 0.1f, 0.15f, 1.0f}}};  // Dark blue-gray
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;
    
    vkCmdBeginRenderPass(commandBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    // 6. Set viewport and scissor (dynamic state)
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchainExtent.width);
    viewport.height = static_cast<float>(swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &viewport);
    
    VkScissor scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchainExtent;
    vkCmdSetScissor(commandBuffers[currentFrame], 0, 1, &scissor);
    
    // 7. Bind pipeline
    vkCmdBindPipeline(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    
    // 8. Bind vertex buffer
    VkBuffer vertexBuffers[] = { vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffers[currentFrame], 0, 1, vertexBuffers, offsets);
}

void VKRenderer::endFrame() {
    // End render pass
    vkCmdEndRenderPass(commandBuffers[currentFrame]);
    
    // End command buffer recording
    if (vkEndCommandBuffer(commandBuffers[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer");
    }
    
    // Submit command buffer
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;      // Wait for image acquisition
    submitInfo.pWaitDstStageMask = waitStages;         // At which stage to wait
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
    
    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;   // Signal when rendering done
    
    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer");
    }
    
    // Present
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;    // Wait for rendering to finish
    
    VkSwapchainKHR swapchains[] = { swapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &currentImageIndex;
    
    VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreateSwapchain();
    }
    
    // Advance frame index
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
```

### Drawing Commands

Between `beginFrame()` and `endFrame()`, you record draw commands:

```cpp
// In your game loop:
VKRenderer::beginFrame();

// Push MVP matrix
PushConstants push{};
push.mvp = projection * view * model;
vkCmdPushConstants(
    commandBuffers[currentFrame], pipelineLayout,
    VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &push
);

// Draw 3 vertices (triangle)
vkCmdDraw(commandBuffers[currentFrame], 3, 1, 0, 0);

VKRenderer::endFrame();
```

### The Full Initialize and Cleanup

At this point your `initialize()` and `cleanup()` should look like:

```cpp
void VKRenderer::initialize(SDL_Window* window) {
    windowRef = window;
    
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createAllocator();
    createSwapchain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
    createVertexBuffer();
}

void VKRenderer::cleanup() {
    vkDeviceWaitIdle(device);
    
    vmaDestroyBuffer(allocator, vertexBuffer, vertexBufferAllocation);
    
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
    vmaDestroyAllocator(allocator);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}
```

### Destruction Order Matters

Destroy in reverse order of creation. A simple rule: if object A was needed to create object B, destroy B before A.

### Compare to Your Engine's Game Loop

```cpp
// Your current SDLRenderer game loop (Engine.cpp):
while (isRunning) {
    SDLRenderer::clearFrame();       // SDL_RenderClear
    input();                         // SDL_PollEvent
    SceneDB::updateActors();
    SDLRenderer::renderFrame();      // SDL_RenderCopy for each draw request
    SDLRenderer::showFrame();        // SDL_RenderPresent
}

// With VKRenderer it would become:
while (isRunning) {
    input();                         // SDL_PollEvent (same!)
    // update logic...
    
    VKRenderer::beginFrame();        // Acquire image, begin command buffer
    // record draw commands...
    VKRenderer::endFrame();          // Submit, present
}
```

---

## 16. Step 14: Uniforms & Push Constants (MVP Matrix)

To transform 3D objects, you need Model-View-Projection matrices sent to the vertex shader.

### The MVP Matrix

```
Model matrix      → positions object in the world (translate, rotate, scale)
View matrix       → positions the camera
Projection matrix → maps 3D to 2D (perspective or orthographic)

Final position = Projection × View × Model × vertexPosition
```

### Two Ways to Send Data to Shaders

| Method | Size Limit | Speed | Use Case |
|--------|-----------|-------|----------|
| **Push constants** | 128 bytes (guaranteed) | Fastest | MVP matrix, per-object data |
| **Uniform buffers** | Much larger | Fast | Scene-wide data, arrays |

For a single MVP matrix (64 bytes), push constants are ideal.

### Using Push Constants

**Shader side** (already in your `triangle.vert`):
```glsl
layout(push_constant) uniform PushConstants {
    mat4 mvp;
} pushConstants;

void main() {
    gl_Position = pushConstants.mvp * vec4(inPosition, 1.0);
}
```

**CPU side** (between beginFrame/endFrame):
```cpp
#include "glm/gtc/matrix_transform.hpp"

// Build MVP
float aspect = (float)swapchainExtent.width / (float)swapchainExtent.height;

glm::mat4 model = glm::rotate(
    glm::mat4(1.0f),
    rotationAngle,                     // Changes each frame
    glm::vec3(0.0f, 0.0f, 1.0f)       // Rotate around Z axis
);

glm::mat4 view = glm::lookAt(
    glm::vec3(0.0f, 0.0f, 2.0f),      // Camera position
    glm::vec3(0.0f, 0.0f, 0.0f),      // Look at origin
    glm::vec3(0.0f, 1.0f, 0.0f)       // Up vector
);

glm::mat4 proj = glm::perspective(
    glm::radians(45.0f),               // Field of view
    aspect,                             // Aspect ratio
    0.1f,                               // Near plane
    100.0f                              // Far plane
);

// IMPORTANT: Vulkan's Y axis is inverted compared to OpenGL
proj[1][1] *= -1;

PushConstants push{};
push.mvp = proj * view * model;

vkCmdPushConstants(
    commandBuffers[currentFrame],
    pipelineLayout,
    VK_SHADER_STAGE_VERTEX_BIT,
    0,
    sizeof(PushConstants),
    &push
);
```

### Key Concepts

**`proj[1][1] *= -1`**: Vulkan's clip space has Y pointing down (opposite to OpenGL/GLM). This flip corrects it so objects appear right-side-up.

**`glm::lookAt`**: Creates a view matrix. Arguments: camera position, target position, up direction.

**`glm::perspective`**: Creates a perspective projection. Arguments: FOV, aspect ratio, near plane, far plane.

**Rotation animation**: Increment `rotationAngle` each frame (e.g., `rotationAngle += 0.01f`).

### Compare to Your OpenGL Shader

Your old `basic.shader` had:
```glsl
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
gl_Position = projection * view * model * vec4(position, 1.0);
```

In Vulkan with push constants, it's the same math, just sent differently:
```glsl
layout(push_constant) uniform PushConstants { mat4 mvp; } push;
gl_Position = push.mvp * vec4(inPosition, 1.0);
```

We combine P*V*M on the CPU into one matrix for efficiency. You could also send them separately if you need the individual matrices in the shader.

---

## 17. Step 15: Depth Buffer (3D)

Without a depth buffer, triangles are drawn in submission order—back faces can appear in front. A depth buffer stores per-pixel depth so closer fragments win.

### What You Need

1. A `VkImage` for depth data
2. A `VkImageView` for the depth image
3. Update render pass to include a depth attachment
4. Update framebuffers to include the depth image view

### Code

```cpp
// Add to VKRenderer.h
inline static VkImage depthImage = VK_NULL_HANDLE;
inline static VmaAllocation depthImageAllocation = VK_NULL_HANDLE;
inline static VkImageView depthImageView = VK_NULL_HANDLE;

static VkFormat findDepthFormat();
static void createDepthResources();
```

```cpp
// Add to VKRenderer.cpp
VkFormat VKRenderer::findDepthFormat() {
    // D32 is widely supported; D24 with stencil is also common
    // Check what your device supports if you want to be thorough
    return VK_FORMAT_D32_SFLOAT;
}

void VKRenderer::createDepthResources() {
    VkFormat depthFormat = findDepthFormat();
    
    // Create depth image
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
    
    if (vmaCreateImage(allocator, &imageInfo, &allocInfo,
                       &depthImage, &depthImageAllocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create depth image");
    }
    
    // Create depth image view
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

### Updating Render Pass for Depth

When you add a depth buffer, your render pass needs a second attachment:

```cpp
void VKRenderer::createRenderPass() {
    // Color attachment (same as before)
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    // NEW: Depth attachment
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;  // Don't need depth data after rendering
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    // NEW: Depth attachment reference
    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;  // Second attachment
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;  // NEW
    
    // ... dependency same as before ...
    
    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    // ... dependency ...
}
```

### Updating Framebuffers for Depth

```cpp
void VKRenderer::createFramebuffers() {
    swapchainFramebuffers.resize(swapchainImageViews.size());
    
    for (size_t i = 0; i < swapchainImageViews.size(); i++) {
        std::array<VkImageView, 2> attachments = {
            swapchainImageViews[i],  // Color
            depthImageView           // Depth (shared across all framebuffers)
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

### Updating Pipeline for Depth Testing

Add depth-stencil state to `createGraphicsPipeline()`:

```cpp
VkPipelineDepthStencilStateCreateInfo depthStencil{};
depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
depthStencil.depthTestEnable = VK_TRUE;
depthStencil.depthWriteEnable = VK_TRUE;
depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;  // Closer fragments win
depthStencil.depthBoundsTestEnable = VK_FALSE;
depthStencil.stencilTestEnable = VK_FALSE;

// Add to pipeline create info:
pipelineInfo.pDepthStencilState = &depthStencil;
```

### Updating beginFrame for Depth Clear

```cpp
// In beginFrame(), change clear values to include depth:
std::array<VkClearValue, 2> clearValues{};
clearValues[0].color = {{{0.1f, 0.1f, 0.15f, 1.0f}}};
clearValues[1].depthStencil = {1.0f, 0};  // Clear depth to 1.0 (far)

renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
renderPassInfo.pClearValues = clearValues.data();
```

### Key Concepts

**Depth format**: `VK_FORMAT_D32_SFLOAT` is 32-bit float depth. Other options: `D24_UNORM_S8_UINT` (24-bit depth + 8-bit stencil).

**Depth clear value**: 1.0 means "infinitely far." Any rendered fragment will be closer.

**`depthCompareOp = LESS`**: A fragment passes if its depth is LESS than what's in the buffer (closer to camera wins).

**Call order**: `createDepthResources()` should be called after `createSwapchain()` and before `createFramebuffers()`.

---

## 18. Step 16: Rendering a Rotating Cube

Now upgrade from a triangle to a cube. This requires:
- 8 vertices, 36 indices (12 triangles × 3 vertices)
- An index buffer
- Rotation around multiple axes

### Cube Vertex Data

```cpp
// 8 corners of a cube, each with a different color
std::vector<Vertex> vertices = {
    // Front face
    {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},  // 0: front-bottom-left
    {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},  // 1: front-bottom-right
    {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},  // 2: front-top-right
    {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}},  // 3: front-top-left
    // Back face
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}},  // 4: back-bottom-left
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},  // 5: back-bottom-right
    {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},  // 6: back-top-right
    {{-0.5f,  0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}},  // 7: back-top-left
};

// 12 triangles (6 faces × 2 triangles each)
std::vector<uint16_t> indices = {
    // Front
    0, 1, 2,  2, 3, 0,
    // Right
    1, 5, 6,  6, 2, 1,
    // Back
    5, 4, 7,  7, 6, 5,
    // Left
    4, 0, 3,  3, 7, 4,
    // Top
    3, 2, 6,  6, 7, 3,
    // Bottom
    4, 5, 1,  1, 0, 4,
};
```

### Index Buffer

An index buffer lets you reuse vertices. Instead of duplicating vertex data for shared corners, you list vertex indices.

```cpp
// Add to VKRenderer.h
inline static VkBuffer indexBuffer = VK_NULL_HANDLE;
inline static VmaAllocation indexBufferAllocation = VK_NULL_HANDLE;
inline static uint32_t indexCount = 0;

static void createIndexBuffer();
```

```cpp
// Add to VKRenderer.cpp
void VKRenderer::createIndexBuffer() {
    std::vector<uint16_t> indices = {
        0, 1, 2,  2, 3, 0,   // Front
        1, 5, 6,  6, 2, 1,   // Right
        5, 4, 7,  7, 6, 5,   // Back
        4, 0, 3,  3, 7, 4,   // Left
        3, 2, 6,  6, 7, 3,   // Top
        4, 5, 1,  1, 0, 4,   // Bottom
    };
    
    indexCount = static_cast<uint32_t>(indices.size());
    VkDeviceSize bufferSize = sizeof(uint16_t) * indices.size();
    
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                      VMA_ALLOCATION_CREATE_MAPPED_BIT;
    
    VmaAllocationInfo allocationInfo;
    
    if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo,
                        &indexBuffer, &indexBufferAllocation, &allocationInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create index buffer");
    }
    
    memcpy(allocationInfo.pMappedData, indices.data(), bufferSize);
}
```

### Drawing with an Index Buffer

Change `vkCmdDraw` to `vkCmdDrawIndexed`:

```cpp
// In beginFrame or between beginFrame/endFrame:

// Bind index buffer
vkCmdBindIndexBuffer(commandBuffers[currentFrame], indexBuffer, 0, VK_INDEX_TYPE_UINT16);

// Draw indexed
vkCmdDrawIndexed(commandBuffers[currentFrame], indexCount, 1, 0, 0, 0);
//                                              ^^^^^^^^^^
//                                              36 indices = 12 triangles
```

### Cube Rotation

```cpp
// Rotate around Y and X axes for a nice tumbling effect
glm::mat4 model = glm::mat4(1.0f);
model = glm::rotate(model, rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));        // Y axis
model = glm::rotate(model, rotationAngle * 0.7f, glm::vec3(1.0f, 0.0f, 0.0f)); // X axis

// Move camera back so cube is visible
glm::mat4 view = glm::lookAt(
    glm::vec3(0.0f, 0.0f, 3.0f),   // Camera at z=3
    glm::vec3(0.0f, 0.0f, 0.0f),   // Looking at origin
    glm::vec3(0.0f, 1.0f, 0.0f)    // Y is up
);
```

### Key Concepts

**Index buffers save memory**: A cube has 8 unique vertices but 36 index entries (6 faces × 2 triangles × 3 vertices). Without indexing, you'd need 36 vertices with duplicated data.

**`VK_INDEX_TYPE_UINT16`**: 16-bit indices support up to 65535 vertices. Use `VK_INDEX_TYPE_UINT32` for larger meshes.

**`vkCmdDrawIndexed(cmdBuf, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance)`**:
- `indexCount`: How many indices to draw
- `instanceCount`: For instanced rendering (just 1 for now)
- Rest are offsets (0 for simple cases)

---

## 19. Swapchain Recreation (Window Resize)

When the window resizes, the swapchain becomes invalid. You need to recreate it along with everything that depends on it.

### Code

```cpp
// Add to VKRenderer.h
static void recreateSwapchain();
static void cleanupSwapchain();

// Add to VKRenderer.cpp
void VKRenderer::cleanupSwapchain() {
    vkDestroyImageView(device, depthImageView, nullptr);
    vmaDestroyImage(allocator, depthImage, depthImageAllocation);
    
    for (auto framebuffer : swapchainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    for (auto imageView : swapchainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
}

void VKRenderer::recreateSwapchain() {
    // Handle minimization (0 size window)
    int width = 0, height = 0;
    SDL_Vulkan_GetDrawableSize(windowRef, &width, &height);
    while (width == 0 || height == 0) {
        SDL_Vulkan_GetDrawableSize(windowRef, &width, &height);
        SDL_WaitEvent(nullptr);
    }
    
    vkDeviceWaitIdle(device);
    
    cleanupSwapchain();
    
    createSwapchain();
    createImageViews();
    createDepthResources();
    createRenderPass();
    createFramebuffers();
    // Pipeline and command buffers don't need recreation
    // (pipeline uses dynamic viewport, command buffers are re-recorded each frame)
}
```

### Key Concepts

**What needs recreation**: Swapchain, image views, depth resources, render pass, framebuffers.

**What doesn't**: Pipeline (uses dynamic viewport/scissor), command pool/buffers, sync objects, vertex/index buffers.

**`vkDeviceWaitIdle`**: Block until the GPU finishes all work. Necessary before destroying resources the GPU might be using.

---

## 20. Next Steps: 3D Actors

Once you have a working cube renderer, here's the path to integrating with your engine:

### Phase 1: Multiple Objects

Right now you push one MVP and draw one object. To draw multiple:

```cpp
for (each object) {
    PushConstants push{};
    push.mvp = proj * view * object.modelMatrix;
    vkCmdPushConstants(..., &push);
    
    vkCmdBindVertexBuffers(..., object.vertexBuffer, ...);
    vkCmdBindIndexBuffer(..., object.indexBuffer, ...);
    vkCmdDrawIndexed(..., object.indexCount, ...);
}
```

### Phase 2: 3D Actor Components

Extend your `Actor` class with 3D data:

```
Actor
├── position (x, y, z)     ← upgrade from 2D
├── rotation (pitch, yaw, roll)
├── scale (x, y, z)
├── mesh reference          ← NEW: which 3D mesh to draw
└── components (Lua scripts, etc.)
```

### Phase 3: Mesh Loading

Load 3D models from files (OBJ, glTF):
1. Add `tinyobjloader` or `assimp` to `external/`
2. Parse mesh → vertices + indices
3. Upload to GPU buffers
4. Store mesh handle in actor

### Phase 4: Textures

1. Load image (stb_image)
2. Create `VkImage` + `VkImageView` + `VkSampler`
3. Create descriptor sets to bind textures to shaders
4. Update shader to sample from texture

### Phase 5: Lighting

1. Add normals to vertex data
2. Pass light positions via uniform buffers
3. Implement lighting in fragment shader (Phong, PBR, etc.)

### Phase 6: 3D Collision

Replace Box2D with a 3D physics library:
- **Bullet Physics** - Full featured
- **Jolt Physics** - Modern, high performance
- **ReactPhysics3D** - Simpler, good for learning

Or implement your own AABB/sphere collision detection.

### Recommended Learning Order

```
Triangle (done)
    → Cube (this tutorial)
        → Multiple cubes
            → Load OBJ files
                → Textures
                    → Camera controls (FPS/orbit)
                        → Lighting
                            → 3D actors with components
```

Each step builds on the previous one. Take your time with each.

---

## Appendix: Quick Reference

### Common Vulkan Patterns

**Enumerate pattern**:
```cpp
uint32_t count = 0;
vkEnumerate*(handle, &count, nullptr);
std::vector<Type> items(count);
vkEnumerate*(handle, &count, items.data());
```

**Create/Destroy pattern**:
```cpp
VkThingCreateInfo createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_THING_CREATE_INFO;
// ... fill in ...
vkCreateThing(device, &createInfo, nullptr, &thing);
// later:
vkDestroyThing(device, thing, nullptr);
```

### Useful Debug Tools

- **Validation layers**: Add `"VK_LAYER_KHRONOS_validation"` to instance layers during development. Catches API misuse.
- **RenderDoc**: GPU debugger, lets you inspect frames
- **Vulkan Configurator (vkconfig)**: Configure validation layers

### Adding Validation Layers

```cpp
// In createInstance(), add:
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
    createInfo.enabledLayerCount = 0;
#else
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
#endif
```

Install validation layers: `sudo apt install vulkan-validationlayers` (Linux)

### Shader Compilation Script

Create `compile_shaders.sh` in your project root:

```bash
#!/bin/bash
SHADER_DIR="resources/shaders"

for shader in "$SHADER_DIR"/*.vert "$SHADER_DIR"/*.frag; do
    if [ -f "$shader" ]; then
        echo "Compiling $shader..."
        glslc "$shader" -o "$shader.spv"
    fi
done

echo "Done."
```

### Order of Operations Cheat Sheet

**Initialization**:
1. `createInstance()`
2. `createSurface()`
3. `pickPhysicalDevice()`
4. `createLogicalDevice()`
5. `createAllocator()` (VMA)
6. `createSwapchain()`
7. `createImageViews()`
8. `createDepthResources()`
9. `createRenderPass()`
10. `createGraphicsPipeline()`
11. `createFramebuffers()`
12. `createCommandPool()`
13. `createCommandBuffers()`
14. `createSyncObjects()`
15. `createVertexBuffer()`
16. `createIndexBuffer()`

**Per Frame**:
1. Wait for fence
2. Acquire image
3. Reset fence
4. Reset command buffer
5. Begin command buffer
6. Begin render pass
7. Set viewport/scissor
8. Bind pipeline
9. Bind vertex buffer
10. Bind index buffer
11. Push constants (MVP)
12. Draw indexed
13. End render pass
14. End command buffer
15. Submit to queue
16. Present

**Cleanup** (reverse of init):
- Wait for device idle
- Destroy in reverse order

---

Good luck! Start with the triangle, get it spinning, then work your way up to the cube. Ask questions as you go.
