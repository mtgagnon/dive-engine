---
name: Vulkan Tutorial Restructure
overview: Restructure the two existing Vulkan tutorial files into four parts that mirror the official vulkan-tutorial.com progression, adapted for the Dive Engine (SDL2, VKRenderer class, VMA). Mipmaps, MSAA, and Compute Shaders are condensed into a "Future Work" section.
todos:
  - id: part1
    content: "Write Part 1: Drawing a Triangle (replace docs/VULKAN_TUTORIAL_PART1.md)"
    status: in_progress
  - id: part2
    content: "Write Part 2: Working with Data (replace docs/VULKAN_TUTORIAL_PART2.md)"
    status: pending
  - id: part3
    content: "Write Part 3: 3D Rendering (create docs/VULKAN_TUTORIAL_PART3.md)"
    status: pending
  - id: part4
    content: "Write Part 4: Engine Integration & Future Work (create docs/VULKAN_TUTORIAL_PART4.md)"
    status: pending
isProject: false
---

# Vulkan Tutorial Restructure Plan

## Current State

- `**VKRenderer**` (`include/VKRenderer.h`, `src/VKRenderer.cpp`) -- empty stub with `initialize`/`cleanup`/`beginFrame`/`endFrame`
- `**VulkanRenderer**` -- broken earlier attempt (header/cpp mismatch), to be superseded
- `**SDLRenderer**` (`include/SDLRenderer.h`) -- production 2D renderer, used as comparison point
- **Shaders** -- `resources/shaders/triangle.vert`, `triangle.frag` exist
- **Engine** -- Actor/Scene/Component (Lua), Box2D physics, wired to SDLRenderer

## File Changes

- **Replace** `docs/VULKAN_TUTORIAL_PART1.md` -- rewrite as "Drawing a Triangle"
- **Replace** `docs/VULKAN_TUTORIAL_PART2.md` -- rewrite as "Working with Data"
- **Create** `docs/VULKAN_TUTORIAL_PART3.md` -- "3D Rendering"
- **Create** `docs/VULKAN_TUTORIAL_PART4.md` -- "Engine Integration & Future Work"

---

## Part 1: Drawing a Triangle

**File:** `docs/VULKAN_TUTORIAL_PART1.md`

Matches official tutorial's "Drawing a triangle" section. Uses **hardcoded vertices in shader** (no vertex buffers yet).

### Sections

- **Introduction**
  - Vulkan vs SDL Renderer mental model shift (keep existing, valuable context)
  - Architecture overview diagram and object relationships table
- **Setup** (matches official Setup section)
  - Base Code: VKRenderer class skeleton, SDL_WINDOW_VULKAN flag
  - Instance Creation: VkApplicationInfo, SDL_Vulkan_GetInstanceExtensions, MoltenVK portability
  - **Validation Layers (NEW)**: checkValidationLayerSupport(), debug messenger, VK_EXT_debug_utils, NDEBUG toggle, proxy functions for extension loading
  - Physical Devices and Queue Families: enumeration pattern, QueueFamilyIndices with std::optional, findQueueFamilies()
  - Logical Device and Queues: unique queue families via std::set, device extensions, vkGetDeviceQueue
- **Presentation** (matches official Presentation section)
  - Window Surface: SDL_Vulkan_CreateSurface, platform differences SDL hides
  - Swap Chain: querySwapchainSupport(), format/mode/extent selection, present modes table
  - Image Views: VkImageViewCreateInfo, subresource range
- **Graphics Pipeline Basics** (matches official, split into sub-sections)
  - Introduction: pipeline stages diagram, immutability concept
  - Shader Modules: GLSL to SPIR-V with glslc, hardcoded triangle vertices in shader, readFile(), createShaderModule()
  - Fixed Functions: vertex input (empty), input assembly, viewport/scissor (dynamic), rasterizer, multisampling, color blending, pipeline layout (empty)
  - Render Passes: color attachment, subpass, dependency, load/store ops, image layouts
  - Conclusion: VkGraphicsPipelineCreateInfo, destroy shader modules after
- **Drawing** (matches official Drawing section)
  - Framebuffers: one per swapchain image
  - Command Buffers: command pool, allocation, recording
  - Rendering and Presentation: drawFrame(), semaphores + fences, sync flow diagram
  - Frames in Flight: MAX_FRAMES_IN_FLIGHT=2, per-frame sync, currentFrame rotation
  - Full Code Checkpoint: complete VKRenderer.h/cpp at this point

**End result:** Hardcoded colorful triangle on screen with validation layers and frames in flight.

---

## Part 2: Working with Data

**File:** `docs/VULKAN_TUTORIAL_PART2.md`

Matches official "Vertex buffers", "Uniform buffers", and "Texture mapping" sections. Introduces VMA.

### Sections

- **Vertex Buffers**
  - Vertex Input Description: Vertex struct, binding/attribute descriptions, update shader to use `layout(location=0) in`, update pipeline
  - Vertex Buffer Creation with VMA: VmaAllocator setup, host-visible buffer, memcpy
  - Staging Buffers: device-local vs host-visible, staging pattern, single-use command helper
  - Index Buffers: why they save memory, VK_INDEX_TYPE, vkCmdDrawIndexed
- **Uniform Buffers & Push Constants**
  - Push Constants (MVP Matrix): PushConstants struct, VkPushConstantRange, glm perspective/lookAt, Vulkan Y-flip, vkCmdPushConstants
  - Descriptor Set Layout and Buffer: when descriptors vs push constants, VkDescriptorSetLayout, per-frame uniform buffers
  - Descriptor Pool and Sets: allocation, updating, binding
- **Texture Mapping**
  - Images: stb_image loading, VkImage, layout transitions, buffer-to-image copy
  - Image View and Sampler: VkSampler, filtering, addressing
  - Combined Image Sampler: shader sampling, descriptor update, binding
- Full Code Checkpoint

**End result:** Textured, transformed triangle/quad with MVP, VMA memory management.

---

## Part 3: 3D Rendering

**File:** `docs/VULKAN_TUTORIAL_PART3.md` (new)

Essential topics for a working 3D engine.

### Sections

- **Depth Buffering**: depth image/view, render pass depth attachment, framebuffer update, pipeline depth-stencil state, clear depth to 1.0
- **Loading Models**: tinyobjloader, loadModel(), vertex deduplication, rendering a 3D model
- **Swapchain Recreation**: resize/minimize handling, cleanupSwapchain()/recreateSwapchain(), what needs recreation vs what doesn't
- **Rendering a Rotating Cube**: cube vertex/index data, multi-axis rotation, full 3D scene
- Full Code Checkpoint

**End result:** Textured 3D model with depth testing, proper resize handling.

---

## Part 4: Engine Integration & Future Work

**File:** `docs/VULKAN_TUTORIAL_PART4.md` (new)

Bridges Vulkan knowledge to the Dive Engine actor/component system.

### Sections

- **Integrating VKRenderer with the Dive Engine**
  - VKRenderer interface mirroring SDLRenderer (initialize/beginFrame/endFrame/cleanup)
  - Switching renderers in Engine.cpp
  - Drawing multiple objects: per-object push constants, for-each actor draw loop
  - 3D Camera system replacing SDLRenderer::setCameraPosition
  - 3D Actor components: extending Actor with 3D transform, MeshRenderer concept
  - Exposing VKRenderer to Lua via ComponentDB (like SDLRenderer::DrawImg today)
  - Resource pipeline: shader compilation, model/texture loading from resources/
- **Future Work** (brief overview + key snippet each)
  - Generating Mipmaps: concept, vkCmdBlitImage, when to add
  - Multisampling (MSAA): what it does, render pass resolve attachment, when to add
  - Compute Shaders: use cases, compute pipeline concept, when to add
  - Lighting: Phong/PBR overview, normals + light uniforms
  - Shadow Mapping: render-to-depth from light perspective
  - 3D Physics: Jolt/Bullet/ReactPhysics3D vs AABB/sphere collision
- **Recommended Learning Path**: ordered progression from triangle to full 3D engine

---

## Key Design Decisions

- **SDL2 throughout** -- all GLFW references replaced with SDL equivalents
- **VKRenderer as target class** -- instance methods, matching existing stub interface
- **VMA for memory** -- no raw vkAllocateMemory; VMA already in external/
- **Validation layers added early** -- dedicated section in Part 1
- **Hardcoded shader vertices first** -- Part 1 uses in-shader vertices, Part 2 introduces vertex buffers
- **Engine comparison boxes** -- "Compare to SDLRenderer" in each major section
- **Code checkpoints** -- each part ends with complete VKRenderer state
- **Incremental build-up** -- initialize() grows as create*() methods are added; cleanup() reverses

