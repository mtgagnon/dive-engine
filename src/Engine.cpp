//
//  Engine.cpp
//  dive_engine
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
#include "SDLRenderer.h"

using std::cout, std::cin, std::string, std::endl, glm::vec2, std::filesystem::exists;

/// Constructor!
Engine::Engine() {
    // check for files
    if(!exists("resources")) {
        string wd = std::filesystem::current_path();
        cout << "error: " << wd << "/resources/ missing" << '\n';
        exit(0);

    } else if(!exists("resources/game.config")) {
        cout << "error: resources/game.config missing" << '\n';
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

    createWindow(game_title);

    SDLRenderer::initialize(window);
    AudioManager::initialize();
    Input::initialize();

    // INITIAL SCENE
    string initial_scene;
    if(config.HasMember("initial_scene") && config["initial_scene"].IsString()) {
        initial_scene = config["initial_scene"].GetString();
    } else {
        cout << "error: initial_scene unspecified" << '\n';
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

        SDLRenderer::clearFrame();

        input();

        SceneDB::updateActors();

        EventBus::PushChangesToSubList();

        RigidBody::physicsStep();

        SDLRenderer::renderFrame();
        SDLRenderer::showFrame();
        Input::LateUpdate();
    }

    return;
}

/**
    Takes in user input as a string, updates current_input as a string
*/
void Engine::input() {
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                isRunning = false;
                break;
            default:
                Input::ProcessEvent(event);
        }
    }
}

// Creates a window and sets the window pointer
// Allows for future renderer changes without changing the engine
// also abstracts the window creation from the renderer
void Engine::createWindow(const string& game_title) {
    int x_resolution = 640;
    int y_resolution = 360;

    if(exists("resources/rendering.config")) {
        rapidjson::Document renderConfig;
        EngineUtils::ReadJsonFile("resources/rendering.config", renderConfig);
        if(renderConfig.HasMember("x_resolution")) {
            x_resolution = renderConfig["x_resolution"].GetInt();
        }
        if(renderConfig.HasMember("y_resolution")) {
            y_resolution = renderConfig["y_resolution"].GetInt();
        }
    }

    if(SDL_Init(SDL_INIT_VIDEO) != 0) {
        cout << "SDL could not initialize! SDL_ERROR: " << SDL_GetError() << endl;
        exit(0);
    }

    window = SDL_CreateWindow(
        game_title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        x_resolution,
        y_resolution,
        SDL_WINDOW_SHOWN  // Change to SDL_WINDOW_VULKAN when using VKRenderer
    );

    if(!window) {
        cout << "Window could not be created! SDL Error: " << SDL_GetError() << endl;
        exit(0);
    }
}
