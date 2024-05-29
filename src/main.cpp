//
//  main.cpp
//  dive_engine
//
//  Created by Mathurin Gagnon on 1/24/24.
//

#define SDL_MAIN_HANDLED

#include <stdio.h>
#include <iostream>

#include "Engine.h"


using std::cout, std::cin, std::string;

inline static SDL_Renderer* renderer = nullptr;


int main(int argc, char* argv[]){
    
    Engine engine;
    engine.game_loop();
    
    
    
    return 0;
}
