//
//  VKRenderer.cpp
//  dive_engine
//

#include "VKRenderer.h"
#include "SDL_vulkan.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "glm/gtc/matrix_transform.hpp"

#include <iostream>
#include <stdexcept>
#include <set>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <array>

