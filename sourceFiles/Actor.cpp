//
//  Actor.cpp
//  game_engine
//
//  Created by Mathurin Gagnon on 2/12/24.
// 

#include <stdio.h>
#include <iostream>
#include <algorithm>
#include <memory>

#include "EngineUtils.h"
#include "Actor.h"
#include "TemplateData.h"
#include "Rigidbody.h"
#include "lua.h"
#include "LuaBridge.h"

using std::string;

Actor::~Actor() {
    auto rb_it = components_by_type["Rigidbody"];
    for(auto rb : rb_it) { // handle memory leaks
        delete rb.second->instance.cast<RigidBody*>();
    }
}

std::shared_ptr<Actor> Actor::createActor(const rapidjson::Value& actorData){
    std::shared_ptr<Actor> actor = std::make_shared<Actor>(Actor());

    // if the actor has a template load from the template, loads all components (instances) from the parent template
    if(actorData.HasMember("template")) { // if it is a template actor
        string templateName = actorData["template"].GetString();
        TemplateData::loadTemplate(templateName, actor); // fill actor.conponents with instances of the parent, adds component to start things
    }

    // update basic things
    actor->name = actorData.HasMember("name") ? actorData["name"].GetString() : actor->name;
    actor->actor_id = num_actors++;
    
    // if there are no components just return after updating to_start_components
    if(!actorData.HasMember("components")) {
        return actor;
    }
    
    // add all the components or update the components to the actor
    for (auto& componentEntry : actorData["components"].GetObject()) { // won't do run if not there
        
        Component component;
        
        // key and type are guarenteed to be there
        string key = componentEntry.name.GetString();
        string type = componentEntry.value["type"].GetString();

        auto it = actor->components_by_key.find(key); // it is already loaded from templates, but should overwrite with any properties if needed
        
        // Create component and establish Lua inheritance if not already in components
        if(type == "Rigidbody") { // RigidBody do things
            
            RigidBody* rb = new RigidBody(componentEntry.value, actor.get()); // Create a RigidBody instance
            luabridge::push(ComponentDB::GetLuaState(), rb); // Push the instance to the Lua state
            component.instance = luabridge::LuaRef::fromStack(ComponentDB::GetLuaState(), -1);
            
            component.instance["key"] = key; // Set the 'key' in the Lua table
            component.instance["enabled"] = true;
            component.instance["type"] = type;

            actor->addCreatedComponent(component);
            continue; // don't do other things is a special component
        } else if(it == actor->components_by_key.end()) { // no template for this component
            component = {ComponentDB::getEmptyTable()};
            luabridge::LuaRef parent = ComponentDB::getParentRef(type);
            ComponentDB::establishLuaInheritance(component.instance, parent);
            
            component.instance["key"] = key; // Set the 'key' in the Lua table
            component.instance["enabled"] = true;

        } else {
            component = it->second;
        }
        
        createComponent(componentEntry, component);
        actor->injectConvenienceReferences(component.instance);
        
        actor->addCreatedComponent(component);
    }
    
        
    // Return the modified actor
    return actor;
}


void Actor::createComponent(const rapidjson::Value::Member &componentEntry, Component &component) {
    
    for(const auto& property : componentEntry.value.GetObject()) { // go through all vals in the component to override
        string val_name = property.name.GetString();
        
        if (property.value.IsString()) {
            std::string stringValue = property.value.GetString();
            component.instance[val_name] = stringValue; // Set string value in the Lua table
        } else if (property.value.IsInt()) {
            int intValue = property.value.GetInt();
            component.instance[val_name] = intValue; // Set int value in the Lua table
        } else if (property.value.IsBool()) {
            bool boolValue = property.value.GetBool();
            component.instance[val_name] = boolValue; // Set bool value in the Lua table
        } else if (property.value.IsFloat()) {
            float floatValue = property.value.GetFloat();
            component.instance[val_name] = floatValue; // Set float value in the Lua table
        }
    }
    
}


void Actor::addCreatedComponent(Component &component) {
    std::string key = component.instance["key"].cast<std::string>();
    if(components_by_key.find(key) != components_by_key.end()) { // already added
        return;
    }
    
    // Store the component in both the map and the vector
    auto it = components_by_key.insert({key, component}).first; // returns pair (iterator, bool)
    
    std::string type = component.instance["type"].cast<std::string>();
    components_by_type[type].insert({key, &it->second});
    
    // add it to required lifeCycle function vectors
    luabridge::LuaRef onStartRef = (component.instance)["OnStart"];
    if (onStartRef.isFunction()) {
        components_to_start[key] = &it->second;
    }
    
    luabridge::LuaRef onUpdateRef = (component.instance)["OnUpdate"];
    if (onUpdateRef.isFunction()) {
        components_requiring_update[key] = &it->second;
    }
    
    luabridge::LuaRef onLateUpdateRef = (component.instance)["OnLateUpdate"];
    if (onLateUpdateRef.isFunction()) {
        components_requiring_late_update[key] = &it->second;
    }
    
    luabridge::LuaRef onDestroyRef = (component.instance)["OnDestroy"];
    if (onDestroyRef.isFunction()) {
        components_requiring_on_destroy[key] = &it->second;
    }
    
    // collision data
    luabridge::LuaRef onCollisionEnterRef = (component.instance)["OnCollisionEnter"];
    if (onCollisionEnterRef.isFunction()) {
        components_with_collision_enter[key] = &it->second;
    }
    
    luabridge::LuaRef onCollisionExitRef = (component.instance)["OnCollisionExit"];
    if (onCollisionExitRef.isFunction()) {
        components_with_collision_exit[key] = &it->second;
    }
    
    // trigger data
    luabridge::LuaRef onTriggerEnterRef = (component.instance)["OnTriggerEnter"];
    if (onTriggerEnterRef.isFunction()) {
        components_with_trigger_enter[key] = &it->second;
    }
    
    luabridge::LuaRef onTriggerExitRef = (component.instance)["OnTriggerExit"];
    if (onTriggerExitRef.isFunction()) {
        components_with_trigger_exit[key] = &it->second;
    }
}


void Actor::injectConvenienceReferences(luabridge::LuaRef &component_ref){
    component_ref["actor"] = this;
}


std::string Actor::GetName() { 
    return name;
}

int Actor::GetID() { 
    return actor_id;
}


luabridge::LuaRef Actor::GetComponentByKey(const std::string &key) {
    auto component = components_by_key.find(key);
    
    // make sure component isn't destroyed
    return (component != components_by_key.end() && !component->second.destroyed) ? component->second.instance : luabridge::LuaRef(ComponentDB::GetLuaState());
}


luabridge::LuaRef Actor::GetComponent(const std::string &type_name) {
    auto type_components = components_by_type.find(type_name);
    
    luabridge::LuaRef res = luabridge::LuaRef(ComponentDB::GetLuaState());
    
    if(type_components != components_by_type.end()) {
        for(auto component : type_components->second) { // iterator through (key, component) map
            if(!component.second->destroyed) {
                res = component.second->instance;
                break;
            }
        }
    }
    
    // check if component isn't destroyed and the type_components isn't empty
    return res;
}


luabridge::LuaRef Actor::GetComponents(const std::string &type_name) {
    
    // iterator for type -> map(key, Component*)
    auto type_components = components_by_type.find(type_name);
    luabridge::LuaRef result = ComponentDB::getEmptyTable();
    
    int index = 1;
    if(type_components != components_by_type.end()) {
        for(auto component : type_components->second) { // iterator through (key, component) map
            if(!component.second->destroyed) {
                result[index++] = component.second->instance;
            }
        }
    }
    return result; // return empty table if no components of type_name exist
}


bool Actor::compare(const std::shared_ptr<Actor> a, const std::shared_ptr<Actor> b) {
    return a->actor_id > b->actor_id;
}


void Actor::OnStart() {
    for(auto it : components_to_start) {
        Component* component = it.second;
        
        if (component->instance["enabled"].cast<bool>()) { // Make sure the LuaRef is a table
            luabridge::LuaRef onStart = (component->instance)["OnStart"];
            
            try {
                onStart(component->instance); // run the function
            } catch (const luabridge::LuaException& e) {
                EngineUtils::ReportError(name, e);
            }
        }
    }
    
    components_to_start.clear();
}


void Actor::OnUpdate() { 
    for(auto it : components_requiring_update) {
        Component* component = it.second;
        
        if (component->instance.isTable() && component->instance["enabled"].cast<bool>()) { // Make sure the LuaRef is a table
            luabridge::LuaRef onUpdate = (component->instance)["OnUpdate"];

            try {
                onUpdate(component->instance); // run function
            } catch (const luabridge::LuaException& e) {
                EngineUtils::ReportError(name, e);
            }
        }
    }
}


void Actor::OnLateUpdate() { 
    for(auto it : components_requiring_late_update) {
        Component* component = it.second;

        if (component->instance.isTable() && component->instance["enabled"].cast<bool>()) { // Make sure the LuaRef is a table
            luabridge::LuaRef onLateUpdate = (component->instance)["OnLateUpdate"];
            
            try {
                onLateUpdate(component->instance); // run function
            } catch (const luabridge::LuaException& e) {
                EngineUtils::ReportError(name, e);
            }
        }
    }
}

void Actor::OnDestroy() {
    for(auto it : components_requiring_on_destroy) {
        Component* component = it.second;

        if (component->instance.isTable()) { // Make sure the LuaRef is a table
            luabridge::LuaRef OnDestroy = (component->instance)["OnDestroy"];
            
            try {
                OnDestroy(component->instance); // run function
            } catch (const luabridge::LuaException& e) {
                EngineUtils::ReportError(name, e);
            }
        }
    }
}

void Actor::OnCollisionEnter(Collision collision) {
    
    for(auto it : components_with_collision_enter) {
        Component* component = it.second;
        
        if (component->instance.isTable() && component->instance["enabled"].cast<bool>()) { // Make sure the LuaRef is a table
            luabridge::LuaRef OnCollisionEnter = (component->instance)["OnCollisionEnter"];
            
            try {
                OnCollisionEnter(component->instance, collision); // run function
            } catch (const luabridge::LuaException& e) {
                EngineUtils::ReportError(name, e);
            }
        }
    }
}

void Actor::OnCollisionExit(Collision collision) {
    
    for(auto it : components_with_collision_exit) {
        Component* component = it.second;

        if (component->instance.isTable() && component->instance["enabled"].cast<bool>()) { // Make sure the LuaRef is a table
            luabridge::LuaRef OnCollisionExit = (component->instance)["OnCollisionExit"];
            
            try {
                OnCollisionExit(component->instance, collision); // run function
            } catch (const luabridge::LuaException& e) {
                EngineUtils::ReportError(name, e);
            }
        }
    }
}

void Actor::OnTriggerEnter(Collision collision) {
    
    for(auto it : components_with_trigger_enter) {
        Component* component = it.second;
        
        if (component->instance.isTable() && component->instance["enabled"].cast<bool>()) { // Make sure the LuaRef is a table
            luabridge::LuaRef OnTriggerEnter = (component->instance)["OnTriggerEnter"];
            
            try {
                OnTriggerEnter(component->instance, collision); // run function
            } catch (const luabridge::LuaException& e) {
                EngineUtils::ReportError(name, e);
            }
        }
    }
}

void Actor::OnTriggerExit(Collision collision) {
    
    for(auto it : components_with_trigger_exit) {
        Component* component = it.second;

        if (component->instance.isTable() && component->instance["enabled"].cast<bool>()) { // Make sure the LuaRef is a table
            luabridge::LuaRef OnTriggerExit = (component->instance)["OnTriggerExit"];
            
            try {
                OnTriggerExit(component->instance, collision); // run function
            } catch (const luabridge::LuaException& e) {
                EngineUtils::ReportError(name, e);
            }
        }
    }
}


luabridge::LuaRef Actor::addComponent(std::string type){
    components_to_add.emplace_back(ComponentDB::getEmptyTable());

    if(type == "Rigidbody") {
        RigidBody* rb = new RigidBody(this); // Create a RigidBody instance
        luabridge::push(ComponentDB::GetLuaState(), rb); // Push the instance to the Lua state
        components_to_add.back().instance = luabridge::LuaRef::fromStack(ComponentDB::GetLuaState(), -1);
    } else  {
        luabridge::LuaRef parent = ComponentDB::getParentRef(type);
        ComponentDB::establishLuaInheritance(components_to_add.back().instance, parent);
        injectConvenienceReferences(components_to_add.back().instance);
    }
    
    components_to_add.back().instance["key"] = 'r'+std::to_string(num_added_components++); // Set the 'key' in the Lua table
    components_to_add.back().instance["enabled"] = true;
    components_to_add.back().instance["type"] = type;

    return components_to_add.back().instance;
}


void Actor::removeComponent(luabridge::LuaRef component) {
    component["enabled"] = false;
    
    std::string key = component["key"].cast<std::string>();
    auto it = components_by_key.find(key);
    if(it != components_by_key.end()) {
        it->second.destroyed = true;
    }
    components_to_remove.push_back(key);
}


void Actor::setDestroyed() {
    for(auto component : components_by_key) {
        component.second.instance["enabled"] = false;
    }
    
    destroyed = true;
}


void Actor::updateComponentList() {

    for(auto component : components_to_add) { // add components
        addCreatedComponent(component);
    }
    
    components_to_add.clear();

    std::sort(components_to_remove.begin(), components_to_remove.end(), std::greater<std::string>());

    while(!components_to_remove.empty()) {
        std::string key = components_to_remove.back();
        Component* component = &components_by_key[key];
        string type = component->instance["type"].cast<string>();
        
        // edge case of components to add changing and compoennts to remove changing in onDestroy
        // run on destroy for the component
        if(components_requiring_on_destroy.find(key) != components_requiring_on_destroy.end()) {
            if (component->instance.isTable()) { // Make sure the LuaRef is a table
                luabridge::LuaRef OnDestroy = (component->instance)["OnDestroy"];
                
                try {
                    OnDestroy(component->instance); // run function
                } catch (const luabridge::LuaException& e) {
                    EngineUtils::ReportError(name, e);
                }
            }
            
        }
        
        if(component->destroyed == false) { // got undestroyed in OnDestroy
            continue;
        }
        
        // time to erase from all data structures
        components_by_type[type].erase(key); // erase from components by type
        
        components_to_start.erase(key);
        components_requiring_update.erase(key);
        components_requiring_late_update.erase(key);
        components_requiring_on_destroy.erase(key);
        
        components_with_collision_enter.erase(key);
        components_with_collision_exit.erase(key);
        components_with_trigger_enter.erase(key);
        components_with_trigger_exit.erase(key);
        
        //finally erase from main data structure
        components_by_key.erase(key);
    
        components_to_remove.pop_back();
    }
    
    for(auto component : components_to_add) { // add components
        addCreatedComponent(component);
    }
    
    components_to_add.clear();
}
