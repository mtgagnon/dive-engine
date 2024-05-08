//
//  Engine.cpp
//  game_engine
//
//  Created by Mathurin Gagnon on 1/24/24.
//
// ---Commit changes----

#include <iostream>
#include <algorithm>
#include <list>
#include <filesystem>

#include "Engine.h"
#include "AudioManager.h"
#include "Input.h"
#include "SceneDB.h"
#include "EngineUtils.h"
#include "Rigidbody.h"
#include "EventBus.h"

using std::cout, std::cin, std::string, std::endl, glm::vec2, std::filesystem::exists;

/// Constructor!
Engine::Engine() {    
    // check for files
    if(!exists("resources")) {
        cout << "error: resources/ missing";
        exit(0);
        
    } else if(!exists("resources/game.config")) {
        cout << "error: resources/game.config missing";
        exit(0);
        
    }
    
    initialize();
}


void Engine::initialize() {
    rapidjson::Document config;
    EngineUtils::ReadJsonFile("resources/game.config", config);
    
    ComponentDB::initComponentDB(); // necessary for lua
    
    string game_title = "";
    if(config.HasMember("game_title")) {
        game_title = config["game_title"].GetString();
    }
    
    Renderer::initialize(game_title);
    AudioManager::initialize();
    Input::initialize();
    
    // INITIAL SCENE
    string initial_scene;
    if(config.HasMember("initial_scene") && config["initial_scene"].IsString()) {
        initial_scene = config["initial_scene"].GetString();
    } else {
        cout << "error: initial_scene unspecified";
        exit(0);
    }
    
    SceneDB::loadNewScene(initial_scene);
}


void Engine::game_loop() {
    isRunning = true;
    
    while(isRunning) {
        if(newScene) {
            SceneDB::loadScene();
            SceneDB::startScene();
            newScene = false;
        }
        
        Renderer::clearFrame();
        
        input();
        
        SceneDB::updateActors();

        EventBus::PushChangesToSubList();

        RigidBody::physicsStep();
        
        Renderer::renderFrame();
        Renderer::showFrame();
        Input::LateUpdate();
    }
    
    return;
}

/**
    Takes in user input as a string, updates current_input as a string
*/
void Engine::input() {
    SDL_Event event;
    while(Helper::SDL_PollEvent498(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                isRunning = false;
                break;
            default:
                Input::ProcessEvent(event);
        }
    }
    
}

