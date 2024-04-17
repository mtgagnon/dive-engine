//
//  Engine.h
//  game_engine
//
//  Created by Mathurin Gagnon on 1/24/24.
//

#ifndef ENGINE_H
#define ENGINE_H

#include <string>
#include <list>
#include <unordered_set>
#include <utility>

#include "lua.hpp"
#include "SDL2/SDL.h"
#include "rapidjson/document.h"
#include "Renderer.h"
#include "Actor.h"
#include "glm/glm.hpp"
#include "SceneDB.h"

enum GameState {
    UNLOADED,
    INTRO,
    SCENE,
    ENDING
};

class Engine {
public:
    Engine();
    
    void game_loop();
        
    static inline bool newScene = false; // necessary for knowing when to restart game loop

private:
    
    void initialize();
    
    void loadScene();
    
    void createWindow(const std::string &game_title);
    
    void input();
    
    void update();
    
    void render();
    
    void updateCamera();
    
    TTF_Font* text_font = nullptr;    
    SceneDB cur_scene;
        
    GameState state = UNLOADED;
        
    bool isRunning = false;
};

#endif /* ENGINE_H */

