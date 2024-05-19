

# Include RapidJSON
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/external/rapidjson/include/rapidjson" "${CMAKE_SOURCE_DIR}/external/rapidjson/include/")

# Add additional cmake libraries
add_subdirectory("${CMAKE_SOURCE_DIR}/external/box2d" EXCLUDE_FROM_ALL)
add_subdirectory("${CMAKE_SOURCE_DIR}/external/glm")
add_subdirectory("${CMAKE_SOURCE_DIR}/external/LuaBridge")
add_subdirectory("${CMAKE_SOURCE_DIR}/external/rapidjson")
add_subdirectory("${CMAKE_SOURCE_DIR}/external/lua")

# SDL Libraries
message(STATUS "Adding SDL2")
add_subdirectory("${CMAKE_SOURCE_DIR}/external/SDL2" EXCLUDE_FROM_ALL)

message(STATUS "Adding SDL2_image")
add_subdirectory("${CMAKE_SOURCE_DIR}/external/SDL2_image" EXCLUDE_FROM_ALL)

message(STATUS "Adding SDL2_mixer")
add_subdirectory("${CMAKE_SOURCE_DIR}/external/SDL2_mixer" EXCLUDE_FROM_ALL)

message(STATUS "Adding SDL2_ttf")
add_subdirectory("${CMAKE_SOURCE_DIR}/external/SDL2_ttf" EXCLUDE_FROM_ALL)

message(STATUS "Linking subdirectories")

# Link SDL2 and extensions
target_link_libraries("${PROJECT_NAME}"
    PRIVATE 
    "SDL2::SDL2"           # Link SDL2
    "SDL2_image::SDL2_image"  # Link SDL2_image
    "SDL2_mixer::SDL2_mixer"  # Link SDL2_mixer
    "SDL2_ttf::SDL2_ttf"   # Link SDL2_ttf
    "box2d"                # Assuming box2d target is named as such
    "glm::glm"             # Assuming glm target is named as such
    "LuaBridge"            # Assuming LuaBridge target is named as such
    "lua"
)