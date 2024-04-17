//
//  SceneData.cpp
//  game_engine
//
//  Created by Mathurin Gagnon on 2/1/24.
//

#include <filesystem>
#include <iostream>
#include <algorithm>
#include <memory>

#include "EventBus.h"
#include "Engine.h"
#include "rapidjson/document.h"
#include "SceneDB.h"
#include "EngineUtils.h"

using glm::ivec2, glm::vec2, std::filesystem::exists, std::cout, std::sort, std::vector;

void SceneDB::loadNewScene(const std::string& sceneName) {
    Engine::newScene = true;
    SceneDB::sceneName = sceneName;
}

///LOADING SCENE
void SceneDB::loadScene() {
    //    cout << path << std::endl; //debug
    if(!exists("resources/scenes/" + sceneName + ".scene")){
        cout << "error: scene " << sceneName << " is missing";
        exit(0);
        return;
    }
            
    //clear data structures
    actors.clear();
    actors_by_name.clear();
    actors_to_add.clear();
    actors_to_remove.clear();
    
    for(auto actor : permanent_actors) {
        actors.push_back(actor);
        actors_by_name[actor->name].push_back(actor);
    }
    
    rapidjson::Document scene_config;
    EngineUtils::ReadJsonFile("resources/scenes/" + sceneName + ".scene", scene_config);
    
    if(!scene_config.HasMember("actors") || !scene_config["actors"].IsArray()) {
        cout << "error: scene not properly formatted";
        exit(0);
        return;
    }
        
    for(const auto& actorData : scene_config["actors"].GetArray()) { // load in actors
        std::shared_ptr<Actor> newActor = Actor::createActor(actorData);
        actors.push_back(newActor);
        //onStart_actors.push_back(newActor);
        actors_by_name[newActor->name].push_back(newActor);
    }
    
}


void SceneDB::startScene() {
    for(auto actor : actors) { // clear actors onStart queue
        actor->OnStart();
    }

    
//    // add actors
//    for(auto actor : actors_to_add) {
//        if(!actor->destroyed) {
//            actors.push_back(actor);
//        }
//    }
//    
//    // remove actors
//    for(auto actor : actors_to_remove) {
//        actors.erase(actor);
//    }
    
//    actors_to_add.clear();
//    actors_to_remove.clear();
}

void SceneDB::updateActors() {
    // on start
    for(auto actor : actors) { // clear actors onStart queue
        if(!actor->destroyed) {
            actor->OnStart();
        }
    }
    
    // update
    for(auto actor : actors) {
        if(!actor->destroyed) {
            actor->OnUpdate();
        }
    }
    
    // AS WE GO THROUGH THE ACTORS LIST IF THE ACTOR GETS DESTROYED ADD IT TO THE ACTORS_TO_REMOVE
    // late update
    for(auto actor_it = actors.begin(); actor_it != actors.end(); actor_it++) {
        auto actor = *actor_it;
        
        if(!actor->destroyed) {
            actor->OnLateUpdate();
        }
        
        if(actor->destroyed) {
            actors_to_remove.push_back(actor_it);
        }
        
        actor->updateComponentList();
    }
    
    
    // add actors, if they are destroyed add to the to be removed
    for(auto actor_it = actors_to_add.begin(); actor_it != actors_to_add.end(); actor_it++) {
        auto actor = *actor_it;

        if(!actor->destroyed) {
            actors.push_back(actor);
            actor->updateComponentList();
        } else {
            actors_to_remove.push_back(actor_it);
        }
    }
    
    // remove actors
    for(auto it = actors_to_remove.rbegin(); it != actors_to_remove.rend(); it++) { // need to iterate through actors_to_remove in reverse because iterators shift as you delete things before them
        (**it)->OnDestroy();
        actors.erase(*it);
    }
    

    
    actors_to_add.clear();
    actors_to_remove.clear();
}

Actor* SceneDB::find(std::string name) {
    auto actors_with_name = actors_by_name.find(name);
    
    if(actors_with_name == actors_by_name.end()) {
        return nullptr;
    }
    
    for(auto actor : actors_with_name->second) {
        if(!actor->destroyed) {
            return actor.get();
        }
    }
    
    return nullptr;
}

luabridge::LuaRef SceneDB::findAll(std::string name) {
    luabridge::LuaRef result = ComponentDB::getEmptyTable();
        
    auto it = actors_by_name.find(name);
    
    int index = 1;
    for(std::shared_ptr<Actor> actor : it->second) { // iterator through (key, component) map
        if(!actor->destroyed) {
            result[index++] = (*actor);
        }
    }
    
    return result;
}

Actor* SceneDB::instantiate(std::string actor_template_name) {
    std::shared_ptr<Actor> newActor = std::make_shared<Actor>(Actor());;
    TemplateData::loadTemplate(actor_template_name, newActor);
    
    newActor->actor_id = Actor::num_actors++;
    
    actors_to_add.push_back(newActor);
    actors_by_name[newActor->name].push_back(newActor);
    
    return newActor.get();
}


void SceneDB::destroy(Actor* actor) {
    actor->setDestroyed(); // set actor state to destroyed
    
    auto actors_with_name = actors_by_name.find(actor->name); // will be completely updated with all new actors
    
    //remove from actors_of_name and permanent actors, don't need to remove from actors_to_add, just don't add it to actors vector
    for(auto actor_it = actors_with_name->second.begin(); actor_it != actors_with_name->second.end(); actor_it++) {
        
        if((*actor_it)->actor_id == actor->actor_id) {
            actors_with_name->second.erase(actor_it); // remove from other data structs
            break;
        }
    }
    
    
    for(auto actor_it = permanent_actors.begin(); actor_it != permanent_actors.end(); actor_it++) {
        if((*actor_it)->actor_id == actor->actor_id) {
            actors_to_add.erase(actor_it); // remove from other data structs
            break;
        }
    }
    
}


void SceneDB::dontDestroy(Actor* actor) {
    auto actors_of_name = actors_by_name.find(actor->name); // will be completely updated with all new actors
    
    for(auto actor : actors_of_name->second) {
        if(actor->actor_id == actor->actor_id) {
            permanent_actors.push_back(actor); // if here the pointer never goes out of scope so destructor never happens as long as this vector not cleared
            return;
        }
    }
}



