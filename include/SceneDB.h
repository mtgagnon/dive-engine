//
//  SceneData.h
//  game_engine
//
//  Created by Mathurin Gagnon on 2/1/24.
//

#ifndef SCENEDATA_H
#define SCENEDATA_H

#include <stdio.h>
#include <map>
#include <string>
#include <vector>
#include <memory>

#include "Actor.h"
#include "glm/glm.hpp"

class SceneDB {
public:
    
    static void loadNewScene(const std::string& sceneName);
    
    ///loads a .config file from path to get scene data, requires LoadNewscene to be called
    static void loadScene();
    
    static std::vector<std::shared_ptr<Actor>>& getActors() {return actors;}
    
    static void renderScene();
            
    static void startScene();
    
    static void updateActors();
    
    static Actor* instantiate(std::string actor_template_name);
    
    static void destroy(Actor* actor);
    
    static Actor* find(std::string name);

    static luabridge::LuaRef findAll(std::string name);
                
    static std::string getSceneName() {return sceneName;}
    
    static void dontDestroy(Actor* actor);

private:
    
    static inline std::vector<std::shared_ptr<Actor>> actors_to_add;
    static inline std::vector<std::vector<std::shared_ptr<Actor>>::iterator> actors_to_remove;
    static inline std::vector<std::shared_ptr<Actor>> actors;
    
    static inline std::vector<std::shared_ptr<Actor>> permanent_actors;
    static inline std::map<std::string, std::vector<std::shared_ptr<Actor>>> actors_by_name;

    
    static inline std::string sceneName = "";

    };

#endif /* SCENEDATA_H */
