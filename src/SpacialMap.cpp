//
//  SpacialMap.cpp
//  game_engine
//
//  Created by Mathurin Gagnon on 2/18/24.

#include <list>
#include <vector>
#include <algorithm>
#include <unordered_set>

#include "SpacialMap.h"
#include "EngineUtils.h"
#include "glm/glm.hpp"

using glm::vec2, std::vector;

std::vector<uint64_t> SpacialMap::getCells(const Actor& actor, glm::vec2 collider) {
    
//    glm::vec2 top_left = actor.transform_pos - collider/(float)2;
//    glm::vec2 bottom_right = actor.transform_pos + collider/(float)2;

    // Calculate the keys for all corners
    std::vector<uint64_t> keys;
    
    if(cellSize.length() == 0 || collider.length() == 0) {
        return keys;
    }
    
//    keys.push_back(positionToKey(top_left));
//    keys.push_back(positionToKey({top_left.x, bottom_right.y}));
//    keys.push_back(positionToKey({bottom_right.x, top_left.y}));
//    keys.push_back(positionToKey(bottom_right));

    // Remove duplicate keys if any corners fall into the same cell
    std::sort(keys.begin(), keys.end());
    keys.erase(unique(keys.begin(), keys.end()), keys.end());
    
    return keys; // not super worried about copies here
}

uint64_t SpacialMap::positionToKey(glm::vec2 position) {
    int32_t xKey = floor(position.x / cellSize.x);
    int32_t yKey = floor(position.y / cellSize.y);
    return EngineUtils::GetXYNum(xKey, yKey);
}

void SpacialMap::updateActorPos(Actor &actor, std::vector<uint64_t> &oldKeys, glm::vec2 collider) {
    if(cellSize.length() == 0) {
        return;
    }
    
    std::vector<uint64_t> newKeys = getCells(actor, collider); // This should return sorted keys

    // Use sets to find differences more efficiently
    std::unordered_set<uint64_t> oldKeySet(oldKeys.begin(), oldKeys.end());
    std::unordered_set<uint64_t> newKeySet(newKeys.begin(), newKeys.end());

    // Calculate keys to add (present in newKeys but not in oldKeys)
    std::vector<uint64_t> toAdd;
    std::copy_if(newKeySet.begin(), newKeySet.end(), std::back_inserter(toAdd),
                 [&oldKeySet](const uint64_t& key) { return oldKeySet.find(key) == oldKeySet.end(); });

    // Calculate keys to remove (present in oldKeys but not in newKeys)
    std::vector<uint64_t> toRemove;
    std::copy_if(oldKeySet.begin(), oldKeySet.end(), std::back_inserter(toRemove),
                 [&newKeySet](const uint64_t& key) { return newKeySet.find(key) == newKeySet.end(); });

    // Perform batch removals
    for (const auto& key : toRemove) {
        auto& cell = map[key];
        auto it = std::remove(cell.begin(), cell.end(), &actor);
        if (it != cell.end()) {
            cell.erase(it, cell.end());
        }
    }

    // Perform batch additions
    for (const auto& key : toAdd) {
        map[key].push_back(&actor);
    }
}
