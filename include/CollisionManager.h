//
//  CollisionManager.h
//  game_engine
//
//  Created by Mathurin Gagnon on 3/26/24.
//

#ifndef COLLISIONMANAGER_H
#define COLLISIONMANAGER_H

#include <stdio.h>
#include "box2d/box2d.h"
#include "Actor.h"

class Actor; // need to forward declare

class Collision {
public:
    Actor* other; // The other actor involved in the collision.
    b2Vec2 point; // First point of collision (use GetWorldManifold())
    b2Vec2 relative_velocity; // body_a velocity - body_b velocity
    b2Vec2 normal; // (use GetWorldManifold().normal)
};

class ContactListener : public b2ContactListener {
public:
    void BeginContact(b2Contact* contact) override;
    
    void EndContact(b2Contact* contact) override;
};



#endif /* COLLISIONMANAGER_H */
