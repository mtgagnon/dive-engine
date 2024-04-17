//
//  Raycast.cpp
//  game_engine
//
//  Created by Mathurin Gagnon on 3/27/24.
//

#include "Raycast.h"
#include "box2d/box2d.h"
#include "ComponentDB.h"
#include "Rigidbody.h"
#include "Helper.h"

luabridge::LuaRef Raycast::PerformRaycast(b2Vec2 pos, b2Vec2 dir, float dist) { 
    RayCastCallback callback;
    dir.Normalize();
    
    // adds only non destroyed hits to hit_results maybe TODO
    luabridge::LuaRef ref = luabridge::LuaRef(ComponentDB::GetLuaState());
    
    if(RigidBody::physics_world) {
        RigidBody::physics_world->RayCast(&callback, pos, pos + dist*dir);
    }
    
    if(callback.hit_results.size()) {
        ref = callback.hit_results.begin()->second;
    }
    
    return ref;
}

luabridge::LuaRef Raycast::RaycastAll(b2Vec2 pos, b2Vec2 dir, float dist) { 
    RayCastCallback callback;
    dir.Normalize();
    
    // adds only non destroyed hits to hit_results maybe TODO
    luabridge::LuaRef ref = ComponentDB::getEmptyTable();
    if(RigidBody::physics_world) {
        RigidBody::physics_world->RayCast(&callback, pos, pos + dist*dir);
    }
    
    int index = 1;
    for(auto it : callback.hit_results) {
        HitResult hit = it.second;
        ref[index++] = hit;
    }
    
    return ref;
}

float RayCastCallback::ReportFixture(b2Fixture *fixture, const b2Vec2 &point, const b2Vec2 &normal, float fraction) {
    Actor* actor = reinterpret_cast<Actor*>(fixture->GetUserData().pointer);
    
    //TODO unsure what to do for destroyed actors
    if(fraction > 0.0f && !actor->destroyed && (fixture->GetFilterData().categoryBits == CATEGORY_COLLIDER || fixture->GetFilterData().categoryBits == CATEGORY_TRIGGER)) {
        hit_results.emplace(fraction, HitResult(actor, point, normal, fixture->IsSensor()));
        return 1.0f;
    }
    
    return -1.0f;
}

