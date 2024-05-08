#  <#Title#>

cmake_minimum_required(VERSION 3.29)
project(dive_engine)

# Define the source files
file(GLOB MY_SOURCES "src/*.cpp" "include/*.h" "include/*.hpp")
add_executable("${PROJECT_NAME}" ${MY_SOURCES})

# Include RapidJSON
target_include_directories(${PROJECT_NAME} PRIVATE "external/rapidjson/include/rapidjson" "external/rapidjson/include/")

# Add External Libraries Lua doesn't have cmake
add_library("${PROJECT_NAME}_lua" "helper_libs/lua/src/*.cpp" "helper_libs/lua/src/*.c" "helper_libs/lua/src/*.h" "helper_libs/lua/src/*.hpp")

target_include_directories("${PROJECT_NAME}_lua"
    PUBLIC 
    "helper_libs/lua/src"
)


# set C++ version for both targets
target_compile_features("${PROJECT_NAME}" PRIVATE cxx_std_11)
target_compile_features("${PROJECT_NAME}_lua" PRIVATE cxx_std_11)

# Add additional cmake libraries
add_subdirectory("external/box2d" EXCLUDE_FROM_ALL)
add_subdirectory("external/glm" EXCLUDE_FROM_ALL)
add_subdirectory("external/LuaBridge" EXCLUDE_FROM_ALL)
add_subdirectory("external/rapidjson" EXCLUDE_FROM_ALL)

# SDL Libraries
add_subdirectory("external/SDL2" EXCLUDE_FROM_ALL)
add_subdirectory("external/SDL2_image" EXCLUDE_FROM_ALL)
add_subdirectory("external/SDL2_mixer" EXCLUDE_FROM_ALL)
add_subdirectory("external/SDL2_ttf" EXCLUDE_FROM_ALL)


# Link SDL2 and extensions
target_link_libraries("${PROJECT_NAME}"
    PRIVATE 
    "${PROJECT_NAME}_lua"  # Link the custom Lua library
    "SDL2::SDL2"           # Link SDL2
    "SDL2_image::SDL2_image"  # Link SDL2_image
    "SDL2_mixer::SDL2_mixer"  # Link SDL2_mixer
    "SDL2_ttf::SDL2_ttf"   # Link SDL2_ttf
    "box2d"                # Assuming box2d target is named as such
    "glm::glm"                  # Assuming glm target is named as such
    "LuaBridge"            # Assuming LuaBridge target is named as such
)