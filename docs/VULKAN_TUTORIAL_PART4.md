# Vulkan Tutorial for Dive Engine — Part 4: Engine Integration & Future Work

Continuing from [Part 3: 3D Rendering](VULKAN_TUTORIAL_PART3.md). This final part bridges your Vulkan knowledge to the Dive Engine's actor/component system and outlines future enhancements.

## Table of Contents

**Engine Integration**
1. [The VKRenderer Interface](#1-the-vkrenderer-interface)
2. [Switching Renderers in Engine.cpp](#2-switching-renderers-in-enginecpp)
3. [Drawing Multiple Objects](#3-drawing-multiple-objects)
4. [3D Camera System](#4-3d-camera-system)
5. [3D Actor Components](#5-3d-actor-components)
6. [Exposing VKRenderer to Lua](#6-exposing-vkrenderer-to-lua)
7. [Resource Pipeline](#7-resource-pipeline)

**Future Work**
8. [Generating Mipmaps](#8-generating-mipmaps)
9. [Multisampling (MSAA)](#9-multisampling-msaa)
10. [Compute Shaders](#10-compute-shaders)
11. [Lighting](#11-lighting)
12. [Shadow Mapping](#12-shadow-mapping)
13. [3D Physics](#13-3d-physics)
14. [Recommended Learning Path](#14-recommended-learning-path)

---

# Engine Integration

## 1. The VKRenderer Interface

`VKRenderer` was designed to mirror `SDLRenderer`'s interface pattern. Both follow the same lifecycle:

```
initialize(window)  →  per-frame { beginFrame / draw / endFrame }  →  cleanup()
```

| SDLRenderer | VKRenderer | Notes |
|-------------|------------|-------|
| `initialize(SDL_Window*)` | `initialize(SDL_Window*)` | Creates all Vulkan objects |
| `clearFrame()` | `beginFrame()` | Acquires image, begins command buffer |
| `renderFrame()` | (draw calls between begin/end) | Records draw commands |
| `showFrame()` | `endFrame()` | Submits, presents, advances frame |
| `cleanup()` | `cleanup()` | Destroys in reverse order |

The key architectural difference: `SDLRenderer` is a static class with free functions. `VKRenderer` is instance-based. This is intentional — Vulkan state is complex enough that having it scoped to an instance is cleaner.

### Adding a Draw Method

For multi-object rendering, add a `drawMesh` method that records draw commands between `beginFrame` and `endFrame`:

```cpp
// VKRenderer.h
void drawMesh(VkBuffer vertexBuf, VkBuffer indexBuf, uint32_t indexCount,
              const glm::mat4& modelMatrix);
```

`drawMesh` is called between `beginFrame()` and `endFrame()` — it records draw commands into the current frame's command buffer. Each call binds a mesh's vertex and index buffers, computes the final MVP matrix by combining the per-frame view/projection with the per-object model matrix, pushes it as a push constant, and issues an indexed draw. This is the Vulkan equivalent of `SDL_RenderCopy` — one call per visible object:

```cpp
// VKRenderer.cpp
void VKRenderer::drawMesh(VkBuffer vertexBuf, VkBuffer indexBuf,
                          uint32_t idxCount, const glm::mat4& modelMatrix)
{
    VkCommandBuffer cmd = commandBuffers[currentFrame];

    VkBuffer vertexBuffers[] = { vertexBuf };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmd, indexBuf, 0, VK_INDEX_TYPE_UINT32);

    PushConstants push{};
    push.mvp = projMatrix * viewMatrix * modelMatrix;
    vkCmdPushConstants(cmd, pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &push);

    vkCmdDrawIndexed(cmd, idxCount, 1, 0, 0, 0);
}
```

---

## 2. Switching Renderers in Engine.cpp

Your `Engine` class currently creates and uses `SDLRenderer`. To support both renderers:

The `useVulkan` flag controls which renderer to use. Since `VKRenderer` requires `SDL_WINDOW_VULKAN` at window creation time, this decision must happen before the window is created:

```cpp
// Engine.h
#include "SDLRenderer.h"
#include "VKRenderer.h"

class Engine {
    // ...
    bool useVulkan = false;
    VKRenderer vkRenderer;
};
```

The game loop maps cleanly between the two renderers. `beginFrame()` replaces `clearFrame()`, draw calls go between begin/end, and `endFrame()` replaces `renderFrame()` + `showFrame()`. The actor update happens in both paths since game logic is renderer-independent:

```cpp
// Engine.cpp
void Engine::initialize() {
    Uint32 windowFlags = SDL_WINDOW_SHOWN;

    if (useVulkan) {
        windowFlags |= SDL_WINDOW_VULKAN;
    }

    window = SDL_CreateWindow(title.c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height, windowFlags);

    if (useVulkan) {
        vkRenderer.initialize(window);
    } else {
        SDLRenderer::initialize(window);
    }
}

void Engine::gameLoop() {
    while (isRunning) {
        input();
        SceneDB::updateActors();

        if (useVulkan) {
            vkRenderer.beginFrame();
            // record 3D draw commands here
            vkRenderer.endFrame();
        } else {
            SDLRenderer::clearFrame();
            SDLRenderer::renderFrame();
            SDLRenderer::showFrame();
        }
    }
}
```

You could also read a `rendering.config` setting to choose the renderer at startup.

---

## 3. Drawing Multiple Objects

In `SDLRenderer`, each actor calls `DrawImg` individually during the render phase. In Vulkan, the pattern is conceptually identical — iterate all visible actors and issue a draw for each — but all commands are recorded into a single command buffer that gets submitted to the GPU at once. This batching is what gives Vulkan its performance advantage: the driver sees all draw calls at once and can optimize scheduling:

```cpp
void Engine::render3D() {
    vkRenderer.beginFrame();

    for (auto& actor : SceneDB::getActors()) {
        if (!actor.hasMesh()) continue;

        MeshData& mesh = actor.getMesh();
        glm::mat4 model = actor.getModelMatrix();

        vkRenderer.drawMesh(mesh.vertexBuffer, mesh.indexBuffer,
                            mesh.indexCount, model);
    }

    vkRenderer.endFrame();
}
```

### Actor Model Matrix

Each 3D actor computes its model matrix from transform components:

```cpp
glm::mat4 Actor::getModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0, 1, 0));
    model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1, 0, 0));
    model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0, 0, 1));
    model = glm::scale(model, scale);
    return model;
}
```

### Resource Management

Use a cache to avoid loading the same mesh or texture multiple times:

```cpp
class MeshCache {
    std::unordered_map<std::string, MeshData> meshes;
public:
    MeshData& load(const std::string& path, VKRenderer& renderer) {
        if (meshes.find(path) == meshes.end()) {
            meshes[path] = renderer.loadMeshFromFile(path);
        }
        return meshes[path];
    }
};
```

This mirrors how `SDLRenderer` already caches `SDL_Texture*` objects by filename.

---

## 4. 3D Camera System

`SDLRenderer` has a simple 2D camera (`setCameraPosition`, `getZoom`). For 3D, you need a proper camera with position, target, and projection.

### Camera Class

```cpp
class Camera3D {
public:
    glm::vec3 position = {0.0f, 2.0f, 5.0f};
    glm::vec3 target = {0.0f, 0.0f, 0.0f};
    glm::vec3 up = {0.0f, 1.0f, 0.0f};
    float fov = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;

    glm::mat4 getViewMatrix() const {
        return glm::lookAt(position, target, up);
    }

    glm::mat4 getProjectionMatrix(float aspectRatio) const {
        glm::mat4 proj = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
        proj[1][1] *= -1;  // Vulkan Y-flip
        return proj;
    }
};
```

Store the camera on `VKRenderer` and update the view/projection matrices at the start of each frame:

```cpp
// In VKRenderer.h
Camera3D camera;

// In beginFrame():
float aspect = static_cast<float>(swapchainExtent.width) /
               static_cast<float>(swapchainExtent.height);
viewMatrix = camera.getViewMatrix();
projMatrix = camera.getProjectionMatrix(aspect);
```

### FPS Camera Controls

For an FPS-style camera that responds to keyboard/mouse:

```cpp
void Camera3D::processKeyboard(const glm::vec3& direction, float deltaTime, float speed) {
    glm::vec3 front = glm::normalize(target - position);
    glm::vec3 right = glm::normalize(glm::cross(front, up));

    if (direction.z > 0) position += front * speed * deltaTime;  // W
    if (direction.z < 0) position -= front * speed * deltaTime;  // S
    if (direction.x > 0) position += right * speed * deltaTime;  // D
    if (direction.x < 0) position -= right * speed * deltaTime;  // A

    target = position + front;
}
```

### Compare to SDLRenderer

```cpp
// SDLRenderer: 2D camera offset
SDLRenderer::setCameraPosition(x, y);
float zoom = SDLRenderer::getZoom();

// VKRenderer: full 3D camera
vkRenderer.camera.position = {0, 5, 10};
vkRenderer.camera.target = {0, 0, 0};
// View/projection matrices computed automatically each frame
```

---

## 5. 3D Actor Components

Your current actor system uses Lua components with a thin C++ `Component` wrapper. To add 3D support, extend the `Actor` class:

### Extending Actor for 3D

```cpp
// In Actor.h — add 3D transform fields alongside existing 2D ones
class Actor {
    // Existing 2D fields (keep for backward compatibility)
    float x = 0, y = 0;
    float rotation_2d = 0;
    float scaleX = 1, scaleY = 1;

    // New 3D fields
    glm::vec3 position3D = {0, 0, 0};
    glm::vec3 rotation3D = {0, 0, 0};  // Euler angles in degrees
    glm::vec3 scale3D = {1, 1, 1};

    // 3D rendering
    std::string meshPath;      // e.g. "resources/models/crate.obj"
    std::string texturePath;   // e.g. "resources/images/crate.png"

    bool hasMesh() const { return !meshPath.empty(); }

    glm::mat4 getModelMatrix() const {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), position3D);
        model = glm::rotate(model, glm::radians(rotation3D.y), {0, 1, 0});
        model = glm::rotate(model, glm::radians(rotation3D.x), {1, 0, 0});
        model = glm::rotate(model, glm::radians(rotation3D.z), {0, 0, 1});
        model = glm::scale(model, scale3D);
        return model;
    }
};
```

### MeshRenderer Component Concept

In a component-based architecture, rendering becomes a component rather than a hardcoded actor feature:

```
Actor
├── Transform3D (position, rotation, scale)
├── MeshRenderer (mesh reference, material/texture)
├── Rigidbody3D (physics — future)
└── Lua scripts (game logic)
```

The `MeshRenderer` holds references to cached GPU resources:

```cpp
struct MeshRendererData {
    MeshData* mesh;       // From MeshCache
    TextureData* texture; // From TextureCache
};
```

During the render loop, iterate actors with `MeshRenderer` data and issue draw calls.

---

## 6. Exposing VKRenderer to Lua

Your `ComponentDB` currently exposes `SDLRenderer` draw functions to Lua via LuaBridge. The same namespace-based pattern works for `VKRenderer`. Rather than exposing raw Vulkan calls (which would be dangerous and complex), we expose high-level setters that modify actor properties. The engine's render loop then reads these properties and issues the appropriate Vulkan draw calls:

```cpp
// In ComponentDB.cpp, when setting up Lua bindings:

// Existing SDLRenderer bindings:
luabridge::getGlobalNamespace(lua_state)
    .beginNamespace("SDLRenderer")
    .addFunction("DrawImg", &SDLRenderer::DrawImg)
    .addFunction("DrawUI", &SDLRenderer::DrawUI)
    .endNamespace();

// New VKRenderer 3D bindings:
luabridge::getGlobalNamespace(lua_state)
    .beginNamespace("Renderer3D")
    .addFunction("SetMesh", [](Actor* actor, const std::string& path) {
        actor->meshPath = path;
    })
    .addFunction("SetTexture", [](Actor* actor, const std::string& path) {
        actor->texturePath = path;
    })
    .addFunction("SetPosition", [](Actor* actor, float x, float y, float z) {
        actor->position3D = {x, y, z};
    })
    .addFunction("SetRotation", [](Actor* actor, float pitch, float yaw, float roll) {
        actor->rotation3D = {pitch, yaw, roll};
    })
    .addFunction("SetScale", [](Actor* actor, float x, float y, float z) {
        actor->scale3D = {x, y, z};
    })
    .endNamespace();
```

Then a Lua component script could look like:

```lua
-- resources/component_types/RotatingCube.lua
function OnStart(self)
    Renderer3D.SetMesh(self.actor, "resources/models/crate.obj")
    Renderer3D.SetTexture(self.actor, "resources/images/crate.png")
end

function OnUpdate(self)
    local rot = self.actor.rotation3D
    Renderer3D.SetRotation(self.actor, rot.x, rot.y + 1, rot.z)
end
```

---

## 7. Resource Pipeline

### Shader Compilation

Create a `compile_shaders.sh` script in your project root:

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

Consider integrating this into CMake so shaders compile automatically during builds:

```cmake
# In CMakeLists.txt
find_program(GLSLC glslc)

file(GLOB SHADERS "resources/shaders/*.vert" "resources/shaders/*.frag")
foreach(SHADER ${SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SPV_FILE "${SHADER}.spv")
    add_custom_command(
        OUTPUT ${SPV_FILE}
        COMMAND ${GLSLC} ${SHADER} -o ${SPV_FILE}
        DEPENDS ${SHADER}
        COMMENT "Compiling ${SHADER_NAME}"
    )
    list(APPEND SPV_FILES ${SPV_FILE})
endforeach()
add_custom_target(shaders ALL DEPENDS ${SPV_FILES})
add_dependencies(dive_engine shaders)
```

### Model and Texture Loading

Models load from `resources/models/` and textures from `resources/images/`, matching your existing `SDLRenderer` resource layout. The `MeshCache` and `TextureCache` manage GPU resources and prevent duplicate uploads.

---

# Future Work

These topics are listed in recommended implementation order. Each includes a brief overview, the key Vulkan concept, and when to add it.

## 8. Generating Mipmaps

**What**: Mipmaps are progressively smaller versions of a texture (half resolution each level). The GPU automatically selects the appropriate level based on distance, reducing aliasing artifacts on distant objects.

**Key concept**: After uploading the full-resolution texture, use `vkCmdBlitImage` in a loop to generate each smaller mip level from the previous one. Set `VkSamplerCreateInfo::mipmapMode` to `VK_SAMPLER_MIPMAP_MODE_LINEAR` for smooth transitions between levels.

```cpp
// Pseudocode for mip generation
for (uint32_t i = 1; i < mipLevels; i++) {
    // Transition level i-1 to TRANSFER_SRC
    // Transition level i to TRANSFER_DST
    // Blit from level i-1 to level i (halving dimensions)
    // Transition level i-1 to SHADER_READ_ONLY
}
// Transition last level to SHADER_READ_ONLY
```

**When to add**: After texture mapping works and you notice textures looking grainy or shimmering at distance.

---

## 9. Multisampling (MSAA)

**What**: Multisample anti-aliasing smooths jagged edges by sampling each pixel at multiple points. MSAA at 4x or 8x dramatically improves edge quality.

**Key changes**:
- Query `VkPhysicalDeviceProperties::limits::framebufferColorSampleCounts` to find supported sample counts
- Create a multisampled color image as an offscreen render target
- Update depth image to match the sample count
- Add a resolve attachment to the render pass that converts multisampled → single-sample for presentation
- Update `VkPipelineMultisampleStateCreateInfo::rasterizationSamples`

**When to add**: After core rendering is solid and you want to polish visual quality. MSAA is a "drop-in" quality improvement that doesn't require shader changes.

---

## 10. Compute Shaders

**What**: Compute shaders run arbitrary GPU programs outside the graphics pipeline. Useful for particle systems, post-processing, physics simulations, and any massively parallel workload.

**Key concept**: A compute pipeline uses `VK_PIPELINE_BIND_POINT_COMPUTE` and operates on storage buffers (`VK_DESCRIPTOR_TYPE_STORAGE_BUFFER`). You dispatch work in workgroups rather than drawing triangles.

```cpp
// Simplified compute dispatch
vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, ...);
vkCmdDispatch(cmd, particleCount / 256, 1, 1);
```

**When to add**: When you need GPU-parallel workloads like particle systems with thousands of particles, or GPU-driven culling for large scenes.

---

## 11. Lighting

**What**: Realistic lighting transforms flat-colored meshes into convincing 3D scenes. Common models:
- **Phong/Blinn-Phong**: Ambient + diffuse + specular. Simple and fast.
- **PBR (Physically Based Rendering)**: Uses metalness/roughness for realistic materials.

**Key changes**:
- Add normals to the `Vertex` struct (and update shaders/pipeline)
- Pass light positions and colors via uniform buffers
- Implement lighting math in the fragment shader

```glsl
// Blinn-Phong in fragment shader (simplified)
vec3 lightDir = normalize(lightPos - fragPos);
float diff = max(dot(normal, lightDir), 0.0);
vec3 diffuse = diff * lightColor;
vec3 result = (ambient + diffuse + specular) * objectColor;
```

**When to add**: After model loading works. This is the single biggest visual improvement for a 3D engine.

---

## 12. Shadow Mapping

**What**: Render the scene from the light's perspective into a depth-only texture (shadow map). In the main render pass, compare each fragment's depth from the light's view to determine if it's in shadow.

**Key concept**: Two render passes per frame:
1. Shadow pass: Render to a depth-only framebuffer using the light's view/projection
2. Main pass: Sample the shadow map in the fragment shader

**When to add**: After basic lighting works. Shadows are the next biggest visual improvement after lighting.

---

## 13. 3D Physics

**What**: Replace or supplement Box2D (2D physics) with a 3D physics engine for collision detection and rigid body dynamics.

**Options**:
- **Jolt Physics** — Modern, high performance, used in Horizon Forbidden West
- **Bullet Physics** — Battle-tested, widely used in games and simulations
- **ReactPhysics3D** — Simpler API, good for learning

**Simpler alternative**: Implement your own AABB (axis-aligned bounding box) or sphere collision detection for basic 3D interactions before committing to a full physics engine.

**When to add**: After 3D actors with transforms are working. Start with AABB overlap checks, then consider a physics library when you need gravity, forces, and rigid body dynamics.

---

## 14. Recommended Learning Path

Follow this progression. Each step builds on the previous one:

```
Part 1: Triangle on screen
    ↓
Part 2: Vertex buffers + textures
    ↓
Part 3: Depth buffer + 3D models
    ↓
Engine integration (multiple objects, camera, actors)
    ↓
Lighting (Phong/Blinn-Phong)
    ↓
Shadow mapping
    ↓
Model loading from various formats (glTF)
    ↓
Mipmaps + MSAA (visual polish)
    ↓
Compute shaders (particles, post-processing)
    ↓
3D physics (Jolt/Bullet)
    ↓
Advanced: PBR materials, deferred rendering, instanced drawing
```

### Key Milestones

| Milestone | What You Can Do |
|-----------|-----------------|
| After Part 1 | Hardcoded triangle rendering, validation layers working |
| After Part 2 | Textured geometry with transforms |
| After Part 3 | 3D models with depth, window resize |
| After integration | Multiple 3D actors in a scene, Lua-scriptable |
| After lighting | Convincing 3D scenes with shading |
| After shadows | Professional-looking 3D rendering |

Take your time with each step. Understanding **why** each piece exists is more valuable than rushing to the end.

---

## Quick Reference

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
vkCreateThing(device, &createInfo, nullptr, &thing);
// later:
vkDestroyThing(device, thing, nullptr);
```

### Useful Debug Tools

- **Validation layers**: Catch API misuse during development
- **RenderDoc**: GPU frame debugger — inspect draw calls, textures, buffers
- **Vulkan Configurator (vkconfig)**: Fine-tune validation layer behavior

### Order of Operations Cheat Sheet

**Initialization** (from Part 3):

```
1.  createInstance()
2.  setupDebugMessenger()
3.  createSurface()
4.  pickPhysicalDevice()
5.  createLogicalDevice()
6.  createVmaAllocator()
7.  createSwapchain()
8.  createImageViews()
9.  createDepthResources()
10. createRenderPass()
11. createDescriptorSetLayout()
12. createGraphicsPipeline()
13. createFramebuffers()
14. createCommandPool()
15. createCommandBuffers()
16. createTextureImage()
17. createTextureImageView()
18. createTextureSampler()
19. loadModel()
20. createVertexBuffer()
21. createIndexBuffer()
22. createUniformBuffers()
23. createDescriptorPool()
24. createDescriptorSets()
25. createSyncObjects()
```

**Per Frame**:

```
1.  Wait for fence
2.  Acquire image
3.  Reset fence
4.  Update uniform buffer
5.  Reset command buffer
6.  Begin command buffer
7.  Begin render pass
8.  Set viewport/scissor
9.  Bind pipeline
10. Bind vertex buffer
11. Bind index buffer
12. Bind descriptor sets
13. Push constants (per-object MVP)
14. Draw indexed
15. End render pass
16. End command buffer
17. Submit to queue
18. Present
```

**Cleanup** (reverse of init): Wait for device idle, then destroy everything in reverse order.
