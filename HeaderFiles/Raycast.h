//
//  Raycast.h
//  game_engine
//
//  Created by Mathurin Gagnon on 3/27/24.
//

#ifndef RAYCAST_H
#define RAYCAST_H

#include <stdio.h>
#include <map>
#include "Actor.h"
#include "lua.hpp"
#include "LuaBridge/LuaBridge.h"

class HitResult {
public:
    HitResult(Actor* actor, const b2Vec2& point, const b2Vec2& normal, bool is_trigger)
            : actor(actor), point(point), normal(normal), is_trigger(is_trigger) {}
    
    Actor* actor; // the actor our raycast found.
    b2Vec2 point; // the point at which the raycast struck a fixture of the actor.
    b2Vec2 normal; // the normal vector at the point.
    bool is_trigger; // whether or not the fixture encountered is a trigger (sensor).
};


class RayCastCallback : public b2RayCastCallback {
public:
    RayCastCallback() {}
    float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction) override;

    std::map<float, HitResult> hit_results; // maps fraction along the ray of the hit and the hitResult
};

class Raycast  {
public:
    static luabridge::LuaRef PerformRaycast(b2Vec2 pos, b2Vec2 dir, float dist);
    static luabridge::LuaRef RaycastAll(b2Vec2 pos, b2Vec2 dir, float dist);
};

#endif /* RAYCAST_H */
