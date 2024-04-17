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


using std::cout, std::cin, std::string;

int main(int argc, char* argv[]){
        
    Engine engine;
    engine.game_loop();
    
    return 0;
}
