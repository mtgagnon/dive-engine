# <#Lua 5.4.6 Dive Engine CMake#>

cmake_minimum_required(VERSION 3.29)

project(Lua)

message(STATUS "DEBUG: Running lua cmakelists")

# Add Helper Libraries Lua doesn't have cmake
file(GLOB LUA_SOURCES "src/*.c")

# Since we are embedding lua with LuaBridge in our project we don't need these files reasoning:
#       - lua.c: This file contains the source code for the standalone Lua interpreter. When compiled,
#       - it creates an executable (commonly named lua) that can interpret and run Lua scripts from the command line.
#
#       - luac.c: This file contains the source code for the standalone Lua compiler. When compiled, it creates an 
#       - executable (commonly named luac) that can compile Lua scripts into bytecode.
list(REMOVE_ITEM LUA_SOURCES "src/lua.c" "src/luac.c")

add_library(lua STATIC ${LUA_SOURCES})
# add_library(lua SHARED ${LUA_SOURCES})

target_include_directories("lua"
    PUBLIC 
    "src/"
)

target_compile_features("lua" PRIVATE cxx_std_11)
