# Dive Engine - Game Developer's Guide

Dive Engine is a 2D game engine built with SDL2 and Box2D that uses **Lua scripting** for game logic. You define your game through JSON configuration files, scene files, actor templates, and Lua component scripts.

---

## Table of Contents

- [Project Structure](#project-structure)
- [Configuration Files](#configuration-files)
  - [game.config](#gameconfig)
  - [rendering.config](#renderingconfig)
- [Scenes](#scenes)
- [Actors](#actors)
- [Actor Templates](#actor-templates)
- [Lua Components](#lua-components)
  - [Component Lifecycle](#component-lifecycle)
  - [The `self` Object](#the-self-object)
  - [Collision & Trigger Callbacks](#collision--trigger-callbacks)
- [Lua API Reference](#lua-api-reference)
  - [Debug](#debug)
  - [Actor (finding & managing)](#actor-finding--managing)
  - [Actor Instance (methods on `self.actor`)](#actor-instance)
  - [Scene](#scene)
  - [Input (Keyboard)](#input-keyboard)
  - [Input (Mouse)](#input-mouse)
  - [Input (Controller)](#input-controller)
  - [Image (Rendering)](#image-rendering)
  - [Text](#text)
  - [Audio](#audio)
  - [Camera](#camera)
  - [Application](#application)
  - [Vector2](#vector2)
  - [Rigidbody (Physics)](#rigidbody-physics)
  - [Physics (Raycasting)](#physics-raycasting)
  - [Event (Pub/Sub)](#event-pubsub)
  - [Collision Object](#collision-object)
  - [HitResult Object](#hitresult-object)

---

## Project Structure

Your game must be organized inside a `resources/` directory located in the working directory where the engine executable runs.

```
my_game/
├── dive_engine              (engine executable)
└── resources/
    ├── game.config           (required - game title and initial scene)
    ├── rendering.config      (optional - window size, colors, zoom)
    ├── scenes/
    │   ├── title_screen.scene
    │   └── gameplay.scene
    ├── actor_templates/
    │   ├── Player.template
    │   └── Enemy.template
    ├── component_types/
    │   ├── PlayerController.lua
    │   ├── SpriteRenderer.lua
    │   └── HealthSystem.lua
    ├── images/
    │   ├── player.png
    │   └── background.png
    ├── fonts/
    │   └── MyFont.ttf
    └── audio/
        ├── jump.wav
        └── music.ogg
```

**Important file format notes:**
- Images must be `.png` files
- Fonts must be `.ttf` files
- Audio files can be `.wav` or `.ogg`
- When referencing these assets from Lua, use the **filename without the extension** (e.g., `"player"` not `"player.png"`)

---

## Configuration Files

### game.config

**Required.** A JSON file that tells the engine the game title and which scene to load first.

```json
{
    "game_title": "My Awesome Game",
    "initial_scene": "title_screen"
}
```

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `game_title` | string | No | Text displayed in the window title bar. Defaults to empty. |
| `initial_scene` | string | **Yes** | Name of the first scene to load (matches a `.scene` filename in `resources/scenes/`). |

### rendering.config

**Optional.** Controls window size, background color, and camera zoom.

```json
{
    "x_resolution": 1280,
    "y_resolution": 720,
    "clear_color_r": 100,
    "clear_color_g": 149,
    "clear_color_b": 237,
    "zoom_factor": 1.0
}
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `x_resolution` | int | 640 | Window width in pixels. |
| `y_resolution` | int | 360 | Window height in pixels. |
| `clear_color_r` | int | 255 | Background red (0-255). |
| `clear_color_g` | int | 255 | Background green (0-255). |
| `clear_color_b` | int | 255 | Background blue (0-255). |
| `zoom_factor` | float | 1.0 | Camera zoom level. Values < 1.0 zoom out, > 1.0 zoom in. |

---

## Scenes

A scene is a JSON file (`.scene`) in `resources/scenes/` that defines all the actors present when the scene loads.

**File:** `resources/scenes/title_screen.scene`

```json
{
    "actors": [
        {
            "name": "background",
            "components": {
                "1": {
                    "type": "SpriteRenderer",
                    "sprite": "title_bg"
                }
            }
        },
        {
            "name": "start_prompt",
            "components": {
                "1": {
                    "type": "DrawText",
                    "str_content": "Press Space to Start",
                    "x": 200,
                    "y": 400,
                    "font_name": "MyFont",
                    "font_size": 48,
                    "r": 255,
                    "g": 255,
                    "b": 255,
                    "a": 255
                },
                "2": {
                    "type": "SceneChanger"
                }
            }
        }
    ]
}
```

The top-level object must contain an `"actors"` array. Each element is an [Actor](#actors) object.

---

## Actors

An actor is a game object defined as a JSON object. Actors have a name and a collection of components that define their behavior.

```json
{
    "name": "my_actor",
    "template": "Player",
    "components": {
        "movement": {
            "type": "PlayerController",
            "speed": 5.0
        },
        "visuals": {
            "type": "SpriteRenderer",
            "sprite": "hero"
        }
    }
}
```

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `name` | string | No | The actor's name. Used with `Actor.Find()` and `Actor.FindAll()`. Defaults to empty string. |
| `template` | string | No | Name of an [actor template](#actor-templates) to inherit components from. |
| `components` | object | No | Map of component key to component definition. |

### Component definitions

Each key in the `components` object is a **unique key string** (can be anything: `"1"`, `"movement"`, `"my_rb"`, etc.). The value is a JSON object with:

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `type` | string | **Yes** | Name of the component type. Must match a `.lua` file in `resources/component_types/` or be `"Rigidbody"`. |
| *(other fields)* | string/int/float/bool | No | Override any default property defined in the Lua component prototype. |

When an actor uses a template *and* defines components, the inline component definitions override or add to the template's components. If a component key matches one from the template, the properties in the scene definition take priority.

---

## Actor Templates

Templates are reusable actor blueprints stored as `.template` files in `resources/actor_templates/`. They have the same structure as an actor definition (without the outer array).

**File:** `resources/actor_templates/Player.template`

```json
{
    "name": "player",
    "components": {
        "1": {
            "type": "Rigidbody",
            "has_trigger": true,
            "trigger_type": "circle",
            "trigger_radius": 0.55,
            "width": 1,
            "height": 0.3,
            "gravity_scale": 2
        },
        "2": {
            "type": "PlayerController"
        },
        "3": {
            "type": "SpriteRenderer",
            "sprite": "hero"
        }
    }
}
```

Templates are referenced by name (filename without `.template`). You can instantiate a template at runtime with `Actor.Instantiate("Player")` or reference it in a scene/template JSON with the `"template"` field.

---

## Lua Components

Components are Lua scripts that define behavior. Each file in `resources/component_types/` defines a **global table** whose name matches the filename (without `.lua`).

**File:** `resources/component_types/PlayerController.lua`

```lua
PlayerController = {
    speed = 5.0,
    jump_power = 10,

    OnStart = function(self)
        self.rb = self.actor:GetComponent("Rigidbody")
    end,

    OnUpdate = function(self)
        local horizontal = 0
        if Input.GetKey("right") then
            horizontal = self.speed
        elseif Input.GetKey("left") then
            horizontal = -self.speed
        end

        if Input.GetKeyDown("space") then
            self.rb:SetVelocity(Vector2(horizontal, -self.jump_power))
        else
            self.rb:SetVelocity(Vector2(horizontal, self.rb:GetVelocity().y))
        end
    end
}
```

**Rules:**
1. The global table name **must** match the filename (e.g., `PlayerController.lua` defines `PlayerController = { ... }`).
2. Default property values in the table can be overridden per-instance by the actor's JSON definition.
3. Each component instance gets its own table that inherits from the prototype via Lua metatables.

### Component Lifecycle

The engine calls these functions in a specific order each frame:

| Function | When it's called |
|----------|-----------------|
| `OnStart(self)` | Once, on the first frame after the component is added to the scene. |
| `OnUpdate(self)` | Every frame, after all `OnStart` calls. |
| `OnLateUpdate(self)` | Every frame, after all `OnUpdate` calls. |
| `OnDestroy(self)` | When the actor is destroyed or the component is removed. |

**Frame order:**
1. Scene transition (if triggered)
2. Clear screen
3. Process input events
4. `OnStart` for all newly added components
5. `OnUpdate` for all components
6. `OnLateUpdate` for all components
7. Process actor/component additions and removals
8. Event bus updates
9. Physics step (Box2D)
10. Render frame
11. Present frame

Components are only called when `enabled` is `true` (which is the default).

### The `self` Object

Every component function receives `self` as the first argument. This table contains:

- All properties defined in the Lua prototype and/or overridden by JSON
- `self.actor` - a reference to the owning Actor (see [Actor Instance](#actor-instance))
- `self.key` - the component's unique key string
- `self.type` - the component's type name string
- `self.enabled` - boolean, controls whether lifecycle functions are called

### Collision & Trigger Callbacks

If a component defines these functions and the actor has a Rigidbody, they will be called during physics interactions:

```lua
MyComponent = {
    OnCollisionEnter = function(self, collision)
        Debug.Log("Hit " .. collision.other:GetName())
    end,

    OnCollisionExit = function(self, collision)
        Debug.Log("Stopped touching " .. collision.other:GetName())
    end,

    OnTriggerEnter = function(self, collision)
        Debug.Log("Entered trigger of " .. collision.other:GetName())
    end,

    OnTriggerExit = function(self, collision)
        Debug.Log("Left trigger of " .. collision.other:GetName())
    end
}
```

Colliders only interact with other colliders. Triggers only interact with other triggers. See the [Collision Object](#collision-object) for the data available in the `collision` parameter.

---

## Lua API Reference

### Debug

Print messages to the console.

| Function | Parameters | Returns | Description |
|----------|-----------|---------|-------------|
| `Debug.Log(message)` | `message`: string | - | Prints to stdout. |
| `Debug.LogError(message)` | `message`: string | - | Prints to stderr. |

```lua
Debug.Log("Player position: " .. tostring(pos.x))
Debug.LogError("Something went wrong!")
```

---

### Actor (finding & managing)

Static functions for finding and managing actors in the scene.

| Function | Parameters | Returns | Description |
|----------|-----------|---------|-------------|
| `Actor.Find(name)` | `name`: string | actor or `nil` | Returns the first non-destroyed actor with the given name. |
| `Actor.FindAll(name)` | `name`: string | table (array) | Returns a Lua table of all non-destroyed actors with the given name. |
| `Actor.Instantiate(template)` | `template`: string | actor | Creates a new actor from the named template. Added to scene next frame. |
| `Actor.Destroy(actor)` | `actor`: actor | - | Marks an actor for destruction. Removed at end of frame. |

```lua
local player = Actor.Find("player")
local enemies = Actor.FindAll("enemy")
local bullet = Actor.Instantiate("Bullet")
Actor.Destroy(bullet)
```

---

### Actor Instance

Methods available on an actor reference (e.g., `self.actor` or a result from `Actor.Find()`).

| Function | Parameters | Returns | Description |
|----------|-----------|---------|-------------|
| `actor:GetName()` | - | string | Returns the actor's name. |
| `actor:GetID()` | - | int | Returns the actor's unique numeric ID. |
| `actor:GetComponentByKey(key)` | `key`: string | component or `nil` | Returns the component with the exact key string. |
| `actor:GetComponent(type)` | `type`: string | component or `nil` | Returns the first non-destroyed component of the given type. |
| `actor:GetComponents(type)` | `type`: string | table (array) | Returns all non-destroyed components of the given type. |
| `actor:AddComponent(type)` | `type`: string | component | Adds a new component of the given type. Available next frame. |
| `actor:RemoveComponent(component)` | `component`: component ref | - | Marks a component for removal. `OnDestroy` is called. |

```lua
local rb = self.actor:GetComponent("Rigidbody")
local sprites = self.actor:GetComponents("SpriteRenderer")
local new_comp = self.actor:AddComponent("HealthBar")
self.actor:RemoveComponent(old_comp)
```

---

### Scene

| Function | Parameters | Returns | Description |
|----------|-----------|---------|-------------|
| `Scene.Load(name)` | `name`: string | - | Loads a new scene on the next frame. All current actors are destroyed except those marked with `DontDestroy`. |
| `Scene.GetCurrent()` | - | string | Returns the name of the current scene. |
| `Scene.DontDestroy(actor)` | `actor`: actor | - | Marks an actor to persist across scene transitions. |

```lua
Scene.Load("level_2")
local current = Scene.GetCurrent()
Scene.DontDestroy(self.actor)
```

---

### Input (Keyboard)

Key names are lowercase strings. See the [key name reference](#keyboard-key-names) for all valid values.

| Function | Parameters | Returns | Description |
|----------|-----------|---------|-------------|
| `Input.GetKey(key)` | `key`: string | bool | `true` while the key is held down. |
| `Input.GetKeyDown(key)` | `key`: string | bool | `true` on the single frame the key was pressed. |
| `Input.GetKeyUp(key)` | `key`: string | bool | `true` on the single frame the key was released. |

```lua
if Input.GetKey("d") then
    -- move right continuously
end

if Input.GetKeyDown("space") then
    -- jump (once)
end
```

#### Keyboard Key Names

| Category | Keys |
|----------|------|
| Letters | `"a"` through `"z"` |
| Numbers | `"0"` through `"9"` |
| Arrows | `"up"`, `"down"`, `"left"`, `"right"` |
| Modifiers | `"lshift"`, `"rshift"`, `"lctrl"`, `"rctrl"`, `"lalt"`, `"ralt"` |
| Editing | `"tab"`, `"return"`, `"enter"`, `"backspace"`, `"delete"`, `"insert"`, `"space"` |
| Other | `"escape"`, `"/"`, `";"`, `"="`, `"-"`, `"."`, `","`, `"["`, `"]"`, `"\\"`, `"'"` |

---

### Input (Mouse)

| Function | Parameters | Returns | Description |
|----------|-----------|---------|-------------|
| `Input.GetMousePosition()` | - | vec2 | Returns mouse position in screen pixels (`.x`, `.y`). |
| `Input.GetMouseButton(button)` | `button`: int | bool | `true` while a mouse button is held. |
| `Input.GetMouseButtonDown(button)` | `button`: int | bool | `true` on the frame the button was pressed. |
| `Input.GetMouseButtonUp(button)` | `button`: int | bool | `true` on the frame the button was released. |
| `Input.GetMouseScrollDelta()` | - | float | Vertical scroll amount this frame. |

Mouse button IDs: `1` = left, `2` = middle, `3` = right.

```lua
local mouse_pos = Input.GetMousePosition()
Debug.Log("Mouse at: " .. mouse_pos.x .. ", " .. mouse_pos.y)

if Input.GetMouseButtonDown(1) then
    Debug.Log("Left click!")
end
```

---

### Input (Controller)

Gamepad support via SDL2 game controller API. Controllers are identified by integer IDs (starting at 0) and are automatically detected when connected.

| Function | Parameters | Returns | Description |
|----------|-----------|---------|-------------|
| `Input.GetControllerButton(id, button)` | `id`: int, `button`: string | bool | `true` while button is held. |
| `Input.GetControllerButtonDown(id, button)` | `id`: int, `button`: string | bool | `true` on the frame the button was pressed. |
| `Input.GetControllerButtonUp(id, button)` | `id`: int, `button`: string | bool | `true` on the frame the button was released. |
| `Input.GetControllerAxis(id, axis)` | `id`: int, `axis`: string | float | Axis value from -1.0 to 1.0 (deadzone: 0.3). |

**Button names:** `"a"`, `"b"`, `"x"`, `"y"`, `"back"`, `"guide"`, `"start"`, `"leftstick"`, `"rightstick"`, `"leftshoulder"`, `"rightshoulder"`, `"dpad_up"`, `"dpad_down"`, `"dpad_left"`, `"dpad_right"`

**Axis names:** `"leftx"`, `"lefty"`, `"rightx"`, `"righty"`, `"triggerleft"`, `"triggerright"`

```lua
if Input.GetControllerButtonDown(0, "a") then
    -- player 1 pressed A
end

local move_x = Input.GetControllerAxis(0, "leftx")
```

---

### Image (Rendering)

All image names reference files in `resources/images/` without the `.png` extension.

World-space drawing uses **meters** as units (100 pixels = 1 meter). The camera affects world-space images but not UI images.

| Function | Parameters | Returns | Description |
|----------|-----------|---------|-------------|
| `Image.Draw(name, x, y)` | `name`: string, `x`: float, `y`: float | - | Draw an image in world space at (x, y) with default settings. |
| `Image.DrawEx(name, x, y, rot, sx, sy, px, py, r, g, b, a, order)` | see below | - | Draw with full control over transform, tint, and sorting. |
| `Image.DrawUI(name, x, y)` | `name`: string, `x`: float, `y`: float | - | Draw a UI image in screen space (unaffected by camera). |
| `Image.DrawUIEx(name, x, y, r, g, b, a, order)` | see below | - | Draw a UI image with tint, alpha, and sorting order. |
| `Image.DrawPixel(x, y, r, g, b, a)` | all floats | - | Draw a single pixel at screen coordinates. |

**`Image.DrawEx` parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `name` | string | Image name (without `.png`). |
| `x`, `y` | float | World position in meters. |
| `rot` | float | Rotation in degrees. |
| `sx`, `sy` | float | Scale. Negative values flip the image. |
| `px`, `py` | float | Pivot point (0.0 to 1.0). `(0.5, 0.5)` = center. |
| `r`, `g`, `b` | float | Color tint (0-255). |
| `a` | float | Alpha/opacity (0-255). |
| `order` | float | Sorting order. Lower values render behind higher values. |

**`Image.DrawUIEx` parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `name` | string | Image name. |
| `x`, `y` | float | Screen position in pixels. |
| `r`, `g`, `b` | float | Color tint (0-255). |
| `a` | float | Alpha (0-255). |
| `order` | float | Sorting order. |

```lua
Image.Draw("background", 0, 0)
Image.DrawEx("hero", 5.0, 3.0, 0, 1, 1, 0.5, 0.5, 255, 255, 255, 255, 0)
Image.DrawUI("health_bar", 10, 10)
```

---

### Text

| Function | Parameters | Returns | Description |
|----------|-----------|---------|-------------|
| `Text.Draw(content, x, y, font, size, r, g, b, a)` | see below | - | Draws text on screen (UI space, not affected by camera). |

| Parameter | Type | Description |
|-----------|------|-------------|
| `content` | string | The text to display. |
| `x`, `y` | int | Screen position in pixels. |
| `font` | string | Font name (filename in `resources/fonts/` without `.ttf`). |
| `size` | int | Font size in points. |
| `r`, `g`, `b`, `a` | int | Color and alpha (0-255). |

```lua
Text.Draw("Score: 100", 10, 10, "MyFont", 24, 255, 255, 255, 255)
```

---

### Audio

Audio files are loaded from `resources/audio/`. Both `.wav` and `.ogg` formats are supported. Reference sounds by name without the extension. The engine supports up to 50 simultaneous audio channels.

| Function | Parameters | Returns | Description |
|----------|-----------|---------|-------------|
| `Audio.Play(channel, name, loop)` | `channel`: int, `name`: string, `loop`: bool | - | Plays a sound on the given channel. |
| `Audio.Halt(channel)` | `channel`: int | - | Stops playback on a channel. |
| `Audio.SetVolume(channel, volume)` | `channel`: int, `volume`: float | - | Sets volume for a channel (0-128). |

Channels are integers starting from 0. Each channel plays one sound at a time. Set `loop` to `true` for continuous looping (e.g., background music).

```lua
Audio.Play(0, "background_music", true)
Audio.Play(1, "jump_sfx", false)
Audio.SetVolume(0, 64)
Audio.Halt(0)
```

---

### Camera

The camera controls which part of the world is visible. Camera position is in **world-space meters**.

| Function | Parameters | Returns | Description |
|----------|-----------|---------|-------------|
| `Camera.SetPosition(x, y)` | `x`: float, `y`: float | - | Sets the camera's center position. |
| `Camera.GetPositionX()` | - | float | Returns the camera's X position. |
| `Camera.GetPositionY()` | - | float | Returns the camera's Y position. |
| `Camera.SetZoom(zoom)` | `zoom`: float | - | Sets the zoom level. < 1.0 zooms out, > 1.0 zooms in. |
| `Camera.GetZoom()` | - | float | Returns the current zoom level. |

```lua
Camera.SetPosition(player_pos.x, player_pos.y)
Camera.SetZoom(0.5)
```

---

### Application

| Function | Parameters | Returns | Description |
|----------|-----------|---------|-------------|
| `Application.Quit()` | - | - | Immediately exits the application. |
| `Application.Sleep(ms)` | `ms`: int | - | Pauses execution for the given milliseconds. |
| `Application.GetFrame()` | - | int | Returns the current frame number (starts at 0). |
| `Application.OpenURL(url)` | `url`: string | - | Opens a URL in the system's default browser. |

```lua
if Input.GetKeyDown("escape") then
    Application.Quit()
end

Debug.Log("Frame: " .. Application.GetFrame())
Application.OpenURL("https://example.com")
```

---

### Vector2

A 2D vector type (backed by Box2D's `b2Vec2`). Used for positions, velocities, forces, and directions.

**Constructor:**

```lua
local v = Vector2(3.0, 4.0)
```

**Properties:**

| Property | Type | Description |
|----------|------|-------------|
| `.x` | float | X component. |
| `.y` | float | Y component. |

**Methods:**

| Function | Returns | Description |
|----------|---------|-------------|
| `v:Normalize()` | - | Normalizes the vector in-place. |
| `v:Length()` | float | Returns the magnitude of the vector. |

**Static Functions:**

| Function | Parameters | Returns | Description |
|----------|-----------|---------|-------------|
| `Vector2.Distance(a, b)` | `a`: Vector2, `b`: Vector2 | float | Distance between two points. |
| `Vector2.Dot(a, b)` | `a`: Vector2, `b`: Vector2 | float | Dot product of two vectors. |

**Operators:**

```lua
local sum = v1 + v2        -- addition
local diff = v1 - v2       -- subtraction
local scaled = v1 * 2.0    -- scalar multiplication
```

**Coordinate system:** Positive X is right, positive Y is **down** (standard screen coordinates). Gravity defaults to `(0, 9.8)`.

---

### Rigidbody (Physics)

The Rigidbody is a special built-in component backed by Box2D. It provides physics simulation including gravity, collisions, and triggers.

#### JSON Properties

Set these in the actor/template JSON when defining a Rigidbody component:

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `x` | float | 0.0 | Initial X position (meters). |
| `y` | float | 0.0 | Initial Y position (meters). |
| `body_type` | string | `"dynamic"` | `"dynamic"`, `"static"`, or `"kinematic"`. |
| `precise` | bool | `true` | Enable continuous collision detection (bullet mode). |
| `gravity_scale` | float | 1.0 | Multiplier for gravity. Set to 0 to disable gravity. |
| `density` | float | 1.0 | Mass density. |
| `angular_friction` | float | 0.3 | Angular damping. |
| `rotation` | float | 0.0 | Initial rotation in degrees. |
| `has_collider` | bool | `true` | Whether this body has a solid collider. |
| `has_trigger` | bool | `true` | Whether this body has a trigger (non-solid overlap detector). |
| `collider_type` | string | `"box"` | Shape: `"box"` or `"circle"`. |
| `width` | float | 1.0 | Box collider half-width (used for both collider and phantom). |
| `height` | float | 1.0 | Box collider half-height. |
| `radius` | float | 0.5 | Circle collider radius. |
| `friction` | float | 0.3 | Surface friction. |
| `bounciness` | float | 0.3 | Restitution (bounciness). |
| `trigger_type` | string | `"box"` | Trigger shape: `"box"` or `"circle"`. |
| `trigger_width` | float | 1.0 | Box trigger half-width. |
| `trigger_height` | float | 1.0 | Box trigger half-height. |
| `trigger_radius` | float | 0.5 | Circle trigger radius. |

#### Body Types

| Type | Description |
|------|-------------|
| `"dynamic"` | Affected by forces and gravity. Moves and collides. |
| `"static"` | Never moves. Good for walls, floors, platforms. |
| `"kinematic"` | Moves programmatically (via velocity), not affected by forces. Good for moving platforms. |

#### Lua Methods

Access a Rigidbody from another component:

```lua
local rb = self.actor:GetComponent("Rigidbody")
```

| Function | Parameters | Returns | Description |
|----------|-----------|---------|-------------|
| `rb:GetPosition()` | - | Vector2 | Current position in meters. |
| `rb:GetRotation()` | - | float | Current rotation in degrees. |
| `rb:GetVelocity()` | - | Vector2 | Current linear velocity. |
| `rb:GetAngularVelocity()` | - | float | Current angular velocity in degrees/sec. |
| `rb:GetGravityScale()` | - | float | Current gravity scale. |
| `rb:GetUpDirection()` | - | Vector2 | Normalized up direction of the body. |
| `rb:GetRightDirection()` | - | Vector2 | Normalized right direction of the body. |
| `rb:AddForce(force)` | `force`: Vector2 | - | Applies a force to the center of mass. |
| `rb:SetVelocity(vel)` | `vel`: Vector2 | - | Directly sets linear velocity. |
| `rb:SetPosition(pos)` | `pos`: Vector2 | - | Teleports the body to a new position. |
| `rb:SetRotation(degrees)` | `degrees`: float | - | Sets rotation in degrees (clockwise). |
| `rb:SetAngularVelocity(degrees)` | `degrees`: float | - | Sets angular velocity in degrees/sec. |
| `rb:SetGravityScale(scale)` | `scale`: float | - | Changes gravity multiplier at runtime. |
| `rb:SetUpDirection(dir)` | `dir`: Vector2 | - | Rotates so the body's up vector matches `dir`. |
| `rb:SetRightDirection(dir)` | `dir`: Vector2 | - | Rotates so the body's right vector matches `dir`. |

```lua
OnUpdate = function(self)
    local rb = self.actor:GetComponent("Rigidbody")

    if Input.GetKeyDown("space") then
        rb:SetVelocity(Vector2(0, -10))
    end

    rb:AddForce(Vector2(5, 0))
    local pos = rb:GetPosition()
    Image.Draw("player", pos.x, pos.y)
end
```

**Note:** Properties like `width`, `height`, `radius`, `collider_type`, and `trigger_type` can only be set at initialization time (in JSON). They cannot be changed dynamically at runtime.

---

### Physics (Raycasting)

Cast rays into the physics world to detect colliders and triggers.

| Function | Parameters | Returns | Description |
|----------|-----------|---------|-------------|
| `Physics.Raycast(pos, dir, dist)` | `pos`: Vector2, `dir`: Vector2, `dist`: float | HitResult or `nil` | Returns the nearest hit, or `nil` if nothing was hit. |
| `Physics.RaycastAll(pos, dir, dist)` | `pos`: Vector2, `dir`: Vector2, `dist`: float | table (array) | Returns all hits sorted by distance. |

```lua
local hit = Physics.Raycast(rb:GetPosition(), Vector2(0, 1), 2.0)
if hit ~= nil then
    Debug.Log("Hit: " .. hit.actor:GetName())
end

local all_hits = Physics.RaycastAll(start_pos, direction, 10.0)
for i = 1, #all_hits do
    Debug.Log("Hit " .. i .. ": " .. all_hits[i].actor:GetName())
end
```

---

### Event (Pub/Sub)

A publish/subscribe event system for decoupled communication between components.

| Function | Parameters | Returns | Description |
|----------|-----------|---------|-------------|
| `Event.Publish(event, data)` | `event`: string, `data`: LuaRef | - | Fires an event. All subscribers' callbacks are called with the data. |
| `Event.Subscribe(event, component, callback)` | `event`: string, `component`: self, `callback`: function | - | Registers a callback. The callback receives `(self, event_data)`. |
| `Event.Unsubscribe(event, component, callback)` | `event`: string, `component`: self, `callback`: function | - | Removes a subscription. |

**Important:** Subscriptions are processed at the end of the frame. If you subscribe during `OnStart`, the subscription is active starting next frame. Always unsubscribe before the component is destroyed to avoid undefined behavior.

```lua
MyComponent = {
    OnStart = function(self)
        Event.Subscribe("player_died", self, self.OnPlayerDied)
    end,

    OnPlayerDied = function(self, event_data)
        Debug.Log("Player died! Reason: " .. event_data)
    end,

    OnDestroy = function(self)
        Event.Unsubscribe("player_died", self, self.OnPlayerDied)
    end
}

-- From another component:
Event.Publish("player_died", "fell off the map")
```

---

### Collision Object

Passed to `OnCollisionEnter`, `OnCollisionExit`, `OnTriggerEnter`, and `OnTriggerExit` callbacks.

| Property | Type | Description |
|----------|------|-------------|
| `collision.other` | actor | The other actor involved in the collision. |
| `collision.point` | Vector2 | The contact point in world space. |
| `collision.relative_velocity` | Vector2 | The relative velocity of the two bodies at the contact. |
| `collision.normal` | Vector2 | The contact normal vector. |

```lua
OnCollisionEnter = function(self, collision)
    if collision.other:GetName() == "enemy" then
        Debug.Log("Hit an enemy!")
        Actor.Destroy(collision.other)
    end
end
```

---

### HitResult Object

Returned by `Physics.Raycast` and elements of the array from `Physics.RaycastAll`.

| Property | Type | Description |
|----------|------|-------------|
| `hit.actor` | actor | The actor that was hit. |
| `hit.point` | Vector2 | The point where the ray intersected. |
| `hit.normal` | Vector2 | The surface normal at the hit point. |
| `hit.is_trigger` | bool | `true` if the hit fixture is a trigger (sensor). |

---

## Complete Example

Here is a minimal working game with a controllable character and a collectible item.

### resources/game.config

```json
{
    "game_title": "My First Dive Game",
    "initial_scene": "main"
}
```

### resources/rendering.config

```json
{
    "x_resolution": 800,
    "y_resolution": 600,
    "clear_color_r": 135,
    "clear_color_g": 206,
    "clear_color_b": 235
}
```

### resources/scenes/main.scene

```json
{
    "actors": [
        {
            "template": "Player",
            "components": {
                "1": {
                    "type": "Rigidbody",
                    "x": 4,
                    "y": 3
                }
            }
        },
        {
            "name": "floor",
            "components": {
                "floor_rb": {
                    "type": "Rigidbody",
                    "body_type": "static",
                    "x": 5,
                    "y": 8,
                    "width": 20,
                    "height": 1,
                    "has_trigger": false
                },
                "floor_sprite": {
                    "type": "SpriteRenderer",
                    "sprite": "floor_tile"
                }
            }
        }
    ]
}
```

### resources/actor_templates/Player.template

```json
{
    "name": "player",
    "components": {
        "1": {
            "type": "Rigidbody",
            "gravity_scale": 2,
            "has_trigger": false
        },
        "2": {
            "type": "PlayerController"
        },
        "3": {
            "type": "SpriteRenderer",
            "sprite": "hero"
        }
    }
}
```

### resources/component_types/PlayerController.lua

```lua
PlayerController = {
    speed = 8,
    jump_power = 12,

    OnStart = function(self)
        self.rb = self.actor:GetComponent("Rigidbody")
    end,

    OnUpdate = function(self)
        local move_x = 0
        if Input.GetKey("right") then move_x = self.speed end
        if Input.GetKey("left") then move_x = -self.speed end

        local ground = Physics.Raycast(self.rb:GetPosition(), Vector2(0, 1), 0.6)
        if Input.GetKeyDown("space") and ground ~= nil then
            self.rb:SetVelocity(Vector2(move_x, -self.jump_power))
        else
            self.rb:SetVelocity(Vector2(move_x, self.rb:GetVelocity().y))
        end
    end
}
```

### resources/component_types/SpriteRenderer.lua

```lua
SpriteRenderer = {
    sprite = "???",
    r = 255,
    g = 255,
    b = 255,
    a = 255,
    sorting_order = 0,
    scale_x = 1,
    scale_y = 1,

    OnStart = function(self)
        self.pos = Vector2(0, 0)
        self.rot_degrees = 0
    end,

    OnUpdate = function(self)
        self.rb = self.actor:GetComponent("Rigidbody")
        if self.rb ~= nil then
            self.pos = self.rb:GetPosition()
            self.rot_degrees = self.rb:GetRotation()
        end

        Image.DrawEx(self.sprite, self.pos.x, self.pos.y,
            self.rot_degrees, self.scale_x, self.scale_y,
            0.5, 0.5, self.r, self.g, self.b, self.a,
            self.sorting_order)
    end
}
```

Place the required image files (`hero.png`, `floor_tile.png`) in `resources/images/` and run the engine.

---

## Tips

- **Rendering is not automatic.** Actors don't draw themselves. You need a component (like `SpriteRenderer`) that calls `Image.Draw` or `Image.DrawEx` each frame.
- **Rigidbody positions are in meters.** The engine uses 100 pixels per meter for world-space rendering.
- **Component keys must be unique** within an actor. Using the same key in a template and a scene definition lets you override specific properties.
- **Physics runs at a fixed timestep** of 1/60th of a second, independent of render frame rate.
- **Actors instantiated at runtime** via `Actor.Instantiate` don't run their `OnStart` until the next frame.
- **Y-axis points down.** Positive Y is downward. Gravity defaults to `(0, 9.8)`, pulling objects down.
- **`Scene.Load` is deferred.** The new scene loads at the start of the next frame, not immediately.
