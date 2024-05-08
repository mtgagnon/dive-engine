//
//  SpacialMap.h
//  game_engine
//
//  Created by Mathurin Gagnon on 2/18/24.
//

#ifndef SPACIALMAP_H
#define SPACIALMAP_H

#include <stdio.h>
#include <unordered_map>
#include <list>
#include <vector>

#include "glm/glm.hpp"
#include "Actor.h"

// class that maps a uint64_t key generated from a int32_t x and y to a list of actors.
// groups the actors together by location
class SpacialMap {
public:
    
    SpacialMap() {}
    
    SpacialMap(glm::vec2 cellSize) : cellSize(cellSize) {}
        
    std::vector<uint64_t> getCells(const Actor& actor, glm::vec2 collider);
    
    std::list<Actor*>& operator [](uint64_t i) {return map[i];}
    
    bool contains(uint64_t i) {return map.find(i) != map.end();}
        
    uint64_t positionToKey(glm::vec2 position);
    
    void updateActorPos(Actor &actor, std::vector<uint64_t> &oldKeys, glm::vec2 collider);
    
private:
    
    // Hash function to map positions to spatial hashmap keys
    
    std::unordered_map<uint64_t, std::list<Actor*>> map;
    glm::vec2 cellSize;
    
};


#endif /* SPACIALMAP_H */
