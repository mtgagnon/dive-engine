# Dive Engine

A 2D game engine built with SDL2.

### Current Features
- 2D rendering via SDL2
- Lua scripting
- Physics via Box2D
- Audio via SDL2_mixer
- Font rendering via SDL2_ttf

### Planned
- Vulkan rendering backend
- 3D support

---

## Dependencies

### Linux (Ubuntu/Debian)

```bash
sudo apt install \
  clang \
  clang-tidy \
  clang-format \
  cmake \
  libvulkan-dev \
  vulkan-validationlayers \
  glslc \
  mesa-common-dev \
  freeglut3-dev
```

### macOS

Install Xcode command line tools and the [LunarG Vulkan SDK](https://vulkan.lunarg.com/sdk/home#mac).

---

## Building

This project uses [direnv](https://direnv.net/) to expose build commands. After installing direnv and running `direnv allow` in the project root, the `dev` command becomes available.

### Linux

```bash
dev build -l    # configure + compile + run (debug)
dev run -l      # compile + run, skipping configure
dev lint        # run clang-tidy on all source files
```

### macOS

```bash
dev build -m    # configure + compile + run (Xcode)
dev run -m      # compile + run, skipping configure
```

### Without direnv

```bash
# Debug
cmake -G "Unix Makefiles" -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -S . -B builds/build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build builds/build-debug
./builds/build-debug/dive_engine

# Release
cmake -G "Unix Makefiles" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -S . -B builds/build-release -DCMAKE_BUILD_TYPE=Release
cmake --build builds/build-release
./builds/build-release/dive_engine
```

---

Have fun!
