# <#Dive Engine CMAKE#>

cmake_minimum_required(VERSION 3.29)

project(dive_engine)

# set(CMAKE_CXX_STANDARD 11)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS OFF)

# Option for users to choose at configuration time
option(USE_SHARED_LIBS "Build shared libraries by default" ON)

# Set BUILD_SHARED_LIBS based on the user's choice or a default
if(USE_SHARED_LIBS)
    set(BUILD_SHARED_LIBS ON)
else()
    set(BUILD_SHARED_LIBS OFF)
endif()

# Define the source files
# Including Lua and other source files
file(GLOB_RECURSE MY_SOURCES "src/*.cpp")

file(GLOB_RECURSE MY_HEADERS "include/*.h" "include/*.hpp")

# Add executable
add_executable("${PROJECT_NAME}" ${MY_SOURCES} ${MY_HEADERS})

# set C++ version for target
target_compile_features("${PROJECT_NAME}" PRIVATE cxx_std_11)

# Specify the include directories
target_include_directories("${PROJECT_NAME}" PRIVATE
    "${PROJECT_SOURCE_DIR}/include/"
)

# Need to set the directory where the executable is put and the engine is run out of (directory where we search for our resources folder from)
# Currently setting it to the root project directory and not the build directory since we clear xcode builds occasionally and not the root folder
# May change this but for now this is good. 
# set_target_properties("${PROJECT_NAME}" PROPERTIES
# )

include("cmake/externalLibs.cmake")