#ifndef ACTOR_H
#define ACTOR_H

#include <stdio.h>
#include <string>
#include <optional>
#include <vector>
#include <utility>
#include <memory>

#include "rapidjson/document.h"
#include "glm/glm.hpp"
#include "ComponentDB.h"
#include "lua.hpp"
#include "LuaBridge.h"
#include "CollisionManager.h"

class Collision; // need to forward declare

struct Component {
    luabridge::LuaRef instance = luabridge::LuaRef(ComponentDB::GetLuaState()); // init to nil
    bool destroyed = false;
    Component() {}
    
    Component(const luabridge::LuaRef& instance)
        : instance(instance) {}
    
    static bool compare(const Component* a, const Component* b) {
        std::string key_a = a->instance["key"].cast<std::string>();
        std::string key_b = b->instance["key"].cast<std::string>();

        return key_a < key_b;
    }
};


class Actor {
public:
    
    Actor() {}
    
    ~Actor();
    
    static std::shared_ptr<Actor> createActor(const rapidjson::Value &actorData);

    static void createComponent(const rapidjson::Value::Member &componentEntry, Component &component);
    
    static bool compare(const std::shared_ptr<Actor> a, const std::shared_ptr<Actor> b);
    
    //lifecycle functions
    void OnStart();
    void OnUpdate();
    void OnLateUpdate();
    void OnDestroy();
    
    void OnCollisionEnter(Collision collision);
    void OnCollisionExit(Collision collision);
    void OnTriggerEnter(Collision collision);
    void OnTriggerExit(Collision collision);
    
    void addCreatedComponent(Component& component);
    
    void injectConvenienceReferences(luabridge::LuaRef &component_ref);
        
    luabridge::LuaRef addComponent(std::string type);

    void removeComponent(luabridge::LuaRef component);
    
    void setDestroyed();
    
    void updateComponentList();
        
    // actor functions for lua
    std::string GetName();
    int GetID();
    
    luabridge::LuaRef GetComponentByKey(const std::string &key);
    luabridge::LuaRef GetComponent(const std::string &type_name);
    luabridge::LuaRef GetComponents(const std::string &type_name);
    
    std::map<std::string, Component> components_by_key; // name -> component
    std::map<std::string, std::map<std::string, Component*>> components_by_type;
    std::vector<Component> components_to_add;
    std::vector<std::string> components_to_remove; // keys

    std::map<std::string, std::map<std::string, Component*>> life_cycle_funcs;
    
    std::map<std::string, Component*> components_to_start; // names of just added components

    std::map<std::string, Component*> components_requiring_update;
    std::map<std::string, Component*> components_requiring_late_update;
    std::map<std::string, Component*> components_requiring_on_destroy;
    
    std::map<std::string, Component*> components_with_collision_enter;
    std::map<std::string, Component*> components_with_collision_exit;
    std::map<std::string, Component*> components_with_trigger_enter;
    std::map<std::string, Component*> components_with_trigger_exit;

    
    std::string name = "";

    int actor_id;
    bool movement_bounce = false;
    bool destroyed = false;
    
    inline static uint32_t num_actors = 0;
    inline static uint32_t num_added_components = 0;
};

#endif /* ACTOR_H */
