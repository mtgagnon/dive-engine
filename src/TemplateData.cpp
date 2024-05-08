//
//  TemplateData.cpp
//  game_engine
//
//  Created by Mathurin Gagnon on 2/4/24.
//

#include <filesystem>
#include <iostream>
#include <map>

#include "rapidjson/document.h"
#include "TemplateData.h"
#include "EngineUtils.h"
#include "ComponentDB.h"
#include "Rigidbody.h"

using std::cout, std::endl, std::filesystem::exists, glm::ivec2;

void TemplateData::loadTemplate(const std::string& templateName, std::shared_ptr<Actor> child_actor) {
    std::string path = "resources/actor_templates/";
    auto templt = templateLookup.find(templateName);
    
    if(templt == templateLookup.end()){ // if it doesn't exists create it
        if(!exists(path + templateName + ".template")){
            cout << "error: template "  << templateName << " is missing";
            exit(0);
        }
        
        rapidjson::Document template_config;
        EngineUtils::ReadJsonFile(path + templateName + ".template", template_config);

        // emplace actor object into map, key = templateName, val = actor constructor
        std::shared_ptr<Actor> newActor = Actor::createActor(template_config);
        loadedTemplates.push_back(newActor);
        
        templateLookup[templateName] = loadedTemplates.back();
        templt = templateLookup.find(templateName);
    }
    
    std::shared_ptr<Actor> template_actor = templt->second;
    
    child_actor->name = template_actor->name;
    for(auto parent_component : template_actor->components_by_key) {
        Component child_component = parent_component.second; // copy component data over with everything that can be shallow copied
        
        if(parent_component.second.instance["type"].cast<std::string>() == "Rigidbody") { // copy rigidbody and then cast it to a luaRef
            RigidBody* child_rb = new RigidBody(*(parent_component.second.instance.cast<RigidBody*>())); // shallow copy not ok need to change pointer to actor
            child_rb->actor = child_actor.get();
            luabridge::push(ComponentDB::GetLuaState(), child_rb); // Push the instance to the Lua state
            child_component.instance = luabridge::LuaRef::fromStack(ComponentDB::GetLuaState(), -1);
            
        } else {
            child_component.instance = ComponentDB::getEmptyTable(); // new empty table for the instance
            
            ComponentDB::establishLuaInheritance(child_component.instance, parent_component.second.instance);
            
            child_actor->injectConvenienceReferences(child_component.instance);
        }
        child_actor->addCreatedComponent(child_component);
    }
        
}
