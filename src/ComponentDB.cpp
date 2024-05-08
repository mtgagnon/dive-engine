//
//  ComponentDB.cpp
//  game_engine
//
//  Created by Mathurin Gagnon on 3/8/24.
//

#include "ComponentDB.h"
#include "lua.hpp"
#include "LuaBridge.h"
#include "Actor.h"
#include "SceneDB.h"
#include "Helper.h"
#include "Input.h"
#include "Renderer.h"
#include "AudioManager.h"
#include "SceneDB.h"
#include "box2d/box2d.h"
#include "Rigidbody.h"
#include "CollisionManager.h"
#include "Raycast.h"
#include "EventBus.h"

#include <thread>
#include <iostream>
#include <filesystem>

using std::filesystem::exists;

void ComponentDB::initComponentDB() {
    if(!lua_state) {
        lua_state = luaL_newstate();
        luaL_openlibs(lua_state);
        
        luabridge::getGlobalNamespace(lua_state)
            .beginNamespace("Debug")
                .addFunction("Log", ComponentDB::CppLog)
                .addFunction("LogError", ComponentDB::CppLogError)
            .endNamespace();
        
        luabridge::getGlobalNamespace(lua_state)
            .beginClass<b2Vec2>("Vector2")
                .addConstructor<void(*) (float, float)>()
                .addProperty("x", &b2Vec2::x)
                .addProperty("y", &b2Vec2::y)
                .addFunction("Normalize", &b2Vec2::Normalize)
                .addFunction("Length", &b2Vec2::Length)
                .addFunction("__add", &b2Vec2::operator_add)
                .addFunction("__sub", &b2Vec2::operator_sub)
                .addFunction("__mul", &b2Vec2::operator_mult)
                .addStaticFunction("Distance", b2Distance)
                .addStaticFunction("Dot", static_cast<float (*)(const b2Vec2&, const b2Vec2&)>(&b2Dot))
            .endClass();
        
        luabridge::getGlobalNamespace(lua_state)
            .beginClass<Actor>("actor")
                .addFunction("GetName", &Actor::GetName)
                .addFunction("GetID", &Actor::GetID)
                .addFunction("GetComponentByKey", &Actor::GetComponentByKey)
                .addFunction("GetComponent", &Actor::GetComponent)
                .addFunction("GetComponents", &Actor::GetComponents)
                .addFunction("AddComponent", &Actor::addComponent)
                .addFunction("RemoveComponent", &Actor::removeComponent)
            .endClass();
        
        luabridge::getGlobalNamespace(lua_state)
            .beginNamespace("Actor")
                .addFunction("Find", SceneDB::find)
                .addFunction("FindAll", SceneDB::findAll)
                .addFunction("Instantiate", SceneDB::instantiate)
                .addFunction("Destroy", SceneDB::destroy)
            .endNamespace();
        
        luabridge::getGlobalNamespace(lua_state)
            .beginNamespace("Application")
                .addFunction("Quit", ComponentDB::quit)
                .addFunction("Sleep", ComponentDB::sleep)
                .addFunction("GetFrame", ComponentDB::getFrame)
                .addFunction("OpenURL", ComponentDB::openURL)
            .endNamespace();
        
        luabridge::getGlobalNamespace(lua_state)
            .beginClass<glm::vec2>("vec2")
                .addProperty("x", &glm::vec2::x)
                .addProperty("y", &glm::vec2::y)
            .endClass();
        
        luabridge::getGlobalNamespace(lua_state)
            .beginNamespace("Input")
                .addFunction("GetKey", Input::GetKey)
                .addFunction("GetKeyDown", Input::GetKeyDown)
                .addFunction("GetKeyUp", Input::GetKeyUp)
                .addFunction("GetMousePosition", Input::GetMousePosition)
                .addFunction("GetMouseButton", Input::GetMouseButton)
                .addFunction("GetMouseButtonDown", Input::GetMouseButtonDown)
                .addFunction("GetMouseButtonUp", Input::GetMouseButtonUp)
                .addFunction("GetMouseScrollDelta", Input::GetMouseScrollDelta)
                .addFunction("GetControllerButton", Input::GetControllerButton)
                .addFunction("GetControllerButtonDown", Input::GetControllerButtonDown)
                .addFunction("GetControllerButtonUp", Input::GetControllerButtonUp)
                .addFunction("GetControllerAxis", Input::GetControllerAxis)
            .endNamespace();
        

        luabridge::getGlobalNamespace(lua_state)
            .beginNamespace("Audio")
            .addFunction("Play", AudioManager::play)
            .addFunction("Halt", AudioManager::halt)
            .addFunction("SetVolume", AudioManager::setVolume)
            .endNamespace();
        
        luabridge::getGlobalNamespace(lua_state)
            .beginNamespace("Text")
                .addFunction("Draw", Renderer::DrawText)
            .endNamespace();
        
        
        luabridge::getGlobalNamespace(lua_state)
            .beginNamespace("Image")
                .addFunction("DrawUI", Renderer::DrawUI)
                .addFunction("DrawUIEx", Renderer::DrawUIEx)
                .addFunction("Draw", Renderer::DrawImg)
                .addFunction("DrawEx", Renderer::DrawImgEx)
                .addFunction("DrawPixel", Renderer::DrawPixel)
            .endNamespace();
        
        luabridge::getGlobalNamespace(lua_state)
            .beginNamespace("Camera")
                .addFunction("SetPosition", Renderer::setCameraPosition)
                .addFunction("GetPositionX", Renderer::getCameraPositionX)
                .addFunction("GetPositionY", Renderer::getCameraPositionY)
                .addFunction("SetZoom", Renderer::setZoom)
                .addFunction("GetZoom", Renderer::getZoom)
            .endNamespace();
        
        luabridge::getGlobalNamespace(lua_state)
            .beginNamespace("Scene")
                .addFunction("Load", SceneDB::loadNewScene)
                .addFunction("GetCurrent", SceneDB::getSceneName)
                .addFunction("DontDestroy", SceneDB::dontDestroy)
            .endNamespace();
        
        luabridge::getGlobalNamespace(lua_state)
            .beginClass<RigidBody>("Rigidbody")
                .addData("key", &RigidBody::key)
                .addData("type", &RigidBody::component_type)
                .addData("enabled", &RigidBody::enabled)
                .addData("x", &RigidBody::x)
                .addData("y", &RigidBody::y)
                .addData("width", &RigidBody::width) // can only change these properties right after initialization and not dynamically
                .addData("height", &RigidBody::height)
                .addData("body_type", &RigidBody::body_type)
                .addData("precise", &RigidBody::precise)
                .addData("gravity_scale", &RigidBody::gravity_scale)
                .addData("density", &RigidBody::density)
                .addData("angular_friction", &RigidBody::angular_friction)
                .addData("rotation", &RigidBody::deg_rotation)
                .addData("has_collider", &RigidBody::has_collider)
                .addData("has_trigger", &RigidBody::has_trigger)
                .addFunction("GetPosition", &RigidBody::GetPosition)
                .addFunction("GetRotation", &RigidBody::GetRotation)
                .addFunction("OnStart", &RigidBody::OnStart)
                .addFunction("OnDestroy", &RigidBody::OnDestroy)
                .addFunction("AddForce", &RigidBody::AddForce)
                .addFunction("SetVelocity", &RigidBody::SetVelocity)
                .addFunction("SetPosition", &RigidBody::SetPosition)
                .addFunction("SetRotation", &RigidBody::SetRotation)
                .addFunction("SetAngularVelocity", &RigidBody::SetAngularVelocity)
                .addFunction("SetGravityScale", &RigidBody::SetGravityScale)
                .addFunction("SetUpDirection", &RigidBody::SetUpDirection)
                .addFunction("SetRightDirection", &RigidBody::SetRightDirection)
                .addFunction("GetVelocity", &RigidBody::GetVelocity)
                .addFunction("GetAngularVelocity", &RigidBody::GetAngularVelocity)
                .addFunction("GetGravityScale", &RigidBody::GetGravityScale)
                .addFunction("GetUpDirection", &RigidBody::GetUpDirection)
                .addFunction("GetRightDirection", &RigidBody::GetRightDirection)
            .endClass();
        
        luabridge::getGlobalNamespace(lua_state)
            .beginClass<Collision>("collison")
                .addData("other", &Collision::other)
                .addData("point", &Collision::point)
                .addData("relative_velocity", &Collision::relative_velocity)
                .addData("normal", &Collision::normal)
            .endClass();
        
        luabridge::getGlobalNamespace(lua_state)
            .beginClass<HitResult>("HitResult")
                .addConstructor<void(*)(Actor*, const b2Vec2&, const b2Vec2&, bool)>()
                .addProperty("actor", &HitResult::actor)
                .addProperty("point", &HitResult::point) // Ensure you have a way to push/get b2Vec2 to/from Lua
                .addProperty("normal", &HitResult::normal) // Same for b2Vec2
                .addProperty("is_trigger", &HitResult::is_trigger)
            .endClass();
        
        luabridge::getGlobalNamespace(lua_state)
            .beginNamespace("Physics")
                .addFunction("Raycast", Raycast::PerformRaycast)
                .addFunction("RaycastAll", Raycast::RaycastAll)
            .endNamespace();

        luabridge::getGlobalNamespace(lua_state)
            .beginNamespace("Event")
                .addFunction("Publish", EventBus::Publish)
                .addFunction("Subscribe", EventBus::Subscribe)
                .addFunction("Unsubscribe", EventBus::Unsubscribe)
            .endNamespace();
        
    }
}

luabridge::LuaRef ComponentDB::getParentRef(std::string &type) {
    if(!exists("resources/component_types/" + type + ".lua")) {
        std::cout << "error: failed to locate component " << type;
        exit(0);
    } else if (components.find(type) == components.end() && luaL_dofile(lua_state, ("resources/component_types/" + type + ".lua").c_str()) != LUA_OK) {
        std::cout << "problem with lua file " <<  type;
        exit(0);
    } else if (components.find(type) == components.end()) {
        components.insert(type);
    }
    
    return luabridge::getGlobal(lua_state, type.c_str());
}

void ComponentDB::establishLuaInheritance(luabridge::LuaRef& instance_table, luabridge::LuaRef& parent_table) {
    // We must create a metatable to establish inheritance in Lua.
    luabridge::LuaRef new_metatable = luabridge::newTable(lua_state);

    new_metatable["__index"] = parent_table;
    
    // We must use the raw Lua C API (Lua stack) to perform a "setmetatable" operation.
    instance_table.push(lua_state);
    new_metatable.push(lua_state);
    lua_setmetatable(lua_state, -2);
    lua_pop(lua_state, 1);
}


void ComponentDB::CppLog(const std::string &message) {
    std::cout << message << std::endl;
}

void ComponentDB::CppLogError(const std::string &message) {
    std::cerr << message << std::endl;
}

void ComponentDB::quit() {
    exit(0);
}

void ComponentDB::sleep(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

int ComponentDB::getFrame() { 
    return Helper::GetFrameNumber();
}

void ComponentDB::openURL(std::string url) { 
    
    std::string cmd;
#if defined(_WIN32)
        cmd = "start " + url;
#elif defined(__APPLE__)
        cmd = "open " + url;
#elif defined(__linux__)
        cmd = "xdg-open " + url;
#else
#error "Platform not supported!"
#endif
    
    std::system(cmd.c_str());
}

