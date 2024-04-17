//
//  Rigidbody.h
//  game_engine
//
//  Created by Mathurin Gagnon on 3/25/24.
//

#ifndef RIGIDBODY_H
#define RIGIDBODY_H

#include <memory>
#include <stdio.h>

#include "lua.hpp"
#include "LuaBridge.h"
#include "ComponentDB.h"
#include "box2d/box2d.h"
#include "rapidjson/document.h"
#include "Actor.h"
#include "CollisionManager.h"

const static inline uint16 CATEGORY_TRIGGER = 0x0002;
const static inline uint16 CATEGORY_COLLIDER = 0x0004;

class RigidBody {
public:
    RigidBody(const rapidjson::Value &componentEntry, Actor* actor);
    RigidBody(Actor* actor);
    
    void OnStart();
    void OnDestroy();
    
    b2Vec2 GetPosition();
    float GetRotation();
    
    b2Vec2 GetVelocity(); // use b2Body->GetLinearVelocity();
    float GetAngularVelocity(); // use b2Body->GetAngularVelocity()
    float GetGravityScale(); // use b2Body->GetGravityScale();
    b2Vec2 GetUpDirection(); // Return the normalized “up” vector for this body.
    b2Vec2 GetRightDirection(); // Return the normalized “right” vector for this body
    
    void AddForce(b2Vec2 force); // use b2Body->ApplyForceToCenter(vec2, true); in c++
    void SetVelocity(b2Vec2 vel); // use b2Body->SetLinearVelocity(vec2); in c++
    void SetPosition(b2Vec2 pos); // use b2Body->SetTransform() in c++
    void SetRotation(float degrees_clockwise);
    void SetAngularVelocity(float degrees_clockwise);
    void SetGravityScale(float scale); // Use b2Body->SetGravityScale(float) in c++
    void SetUpDirection(b2Vec2 direction);
    void SetRightDirection(b2Vec2 direction);

    static void physicsStep();
    
    void initialializeBodyDef(b2BodyDef &bodyDef, const rapidjson::Value &componentEntry);
    
    void initializeCollider();
    
    void initializeTrigger();
    
    static inline b2World* physics_world = nullptr;
    
    static inline ContactListener* contact_listener = nullptr;

    std::string component_type = "Rigidbody";
    std::string key = "";
    
    std::string body_type = "dynamic";
    std::string collider_type = "box";
    std::string trigger_type = "box";

    
    // Box2D body pointer
    b2Body* body = nullptr;
    Actor* actor = nullptr;
    
    // Setup Properties
    float x = 0.0f;
    float y = 0.0f;
    float gravity_scale = 1.0f;
    float angular_friction = 0.3f;
    float deg_rotation = 0.0f;
    float density = 1.0f;
    
    // collider properties
    float width = 1.0f;
    float height = 1.0f;
    float radius = 0.5f;
    float friction = 0.3f;
    float bounciness = 0.3f;
    
    // trigger properties
    float trigger_width = 1.0f;
    float trigger_height = 1.0f;
    float trigger_radius = 0.5f;
    
    bool has_collider = true;
    bool has_trigger = true;
    bool enabled = true;
    bool precise = true;

};

#endif /* RIGIDBODY_H */
