//
//  main.cpp
//  game_engine
//
//  Created by Mathurin Gagnon on 1/24/24.
//

#define SDL_MAIN_HANDLED

#include <stdio.h>
#include <iostream>

#include "Engine.h"
#include "box2d/box2d.h"


#include "SDL2/SDL.h"
#include "GL/glew.h"

using std::cout, std::cin, std::string;

inline static SDL_Renderer* renderer = nullptr;


int main(int argc, char* argv[]){
    //INITIALIZE
    if(SDL_Init(SDL_INIT_VIDEO)) {
        std::cout << "SDL could not initialize! SDL_ERROR: " << SDL_GetError() << std::endl;
        exit(0);
    }
    
    if (TTF_Init() == -1) {
        std::cerr << "SDL_ttf could not initialize! SDL_ttf Error: " << TTF_GetError() << std::endl;
        exit(0);
    }
    
    SDL_Window* window = SDL_CreateWindow("test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);

    if(!window) {
        std::cerr << "Window could not be created! SDL Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return;
    }
    
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) {
        std::cerr << "Failed to create OpenGL context: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    
    SDL_GL_SetSwapInterval(1);

        
    bool running = true;
    
    while (running) {

        SDL_Event event;
        while(SDL_PollEvent(&event)) {
             running = event.type != SDL_QUIT;
        }
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        SDL_GL_SwapWindow(window);
    }
    
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
//    Engine engine;
//    engine.game_loop();
    
    
    
    return 0;
}
