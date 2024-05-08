//
//  Rigidbody.cpp
//  game_engine
//
//  Created by Mathurin Gagnon on 3/25/24.
//

#include "Rigidbody.h"
#include "rapidjson/document.h"
#include "box2d/box2d.h"
#include "glm/glm.hpp"
#include "Helper.h"
#include "CollisionManager.h"

// Constructor
RigidBody::RigidBody(const rapidjson::Value &componentEntry, Actor* actor) {
    if(!contact_listener) {
        physics_world = new b2World(b2Vec2(0.0f, 9.8f));
        contact_listener = new ContactListener();
        physics_world->SetContactListener(contact_listener);
    }
    
    if (componentEntry.HasMember("x") && componentEntry["x"].IsNumber()) {
        x = componentEntry["x"].GetFloat();
    }

    if (componentEntry.HasMember("y") && componentEntry["y"].IsNumber()) {
        y = componentEntry["y"].GetFloat();
    }

    if (componentEntry.HasMember("body_type") && componentEntry["body_type"].IsString()) {
        body_type = componentEntry["body_type"].GetString();
    }

    if (componentEntry.HasMember("precise") && componentEntry["precise"].IsBool()) {
        precise = componentEntry["precise"].GetBool();
    }

    if (componentEntry.HasMember("gravity_scale") && componentEntry["gravity_scale"].IsNumber()) {
        gravity_scale = componentEntry["gravity_scale"].GetFloat();
    }

    if (componentEntry.HasMember("density") && componentEntry["density"].IsNumber()) {
        density = componentEntry["density"].GetFloat();
    }

    if (componentEntry.HasMember("angular_friction") && componentEntry["angular_friction"].IsNumber()) {
        angular_friction = componentEntry["angular_friction"].GetFloat();
    }

    if (componentEntry.HasMember("rotation") && componentEntry["rotation"].IsNumber()) {
        deg_rotation = componentEntry["rotation"].GetFloat();
    }

    if (componentEntry.HasMember("has_collider") && componentEntry["has_collider"].IsBool()) {
        has_collider = componentEntry["has_collider"].GetBool();
    }

    if (componentEntry.HasMember("has_trigger") && componentEntry["has_trigger"].IsBool()) {
        has_trigger = componentEntry["has_trigger"].GetBool();
    }
    
    if (componentEntry.HasMember("collider_type") && componentEntry["collider_type"].IsString()) {
        collider_type = componentEntry["collider_type"].GetString();
    }
    
    if (componentEntry.HasMember("width") && componentEntry["width"].IsNumber()) {
        width = componentEntry["width"].GetFloat();
    }
    
    if (componentEntry.HasMember("height") && componentEntry["height"].IsNumber()) {
        height = componentEntry["height"].GetFloat();
    }
    
    if (componentEntry.HasMember("radius") && componentEntry["radius"].IsNumber()) {
        radius = componentEntry["radius"].GetFloat();
    }
    
    if (componentEntry.HasMember("friction") && componentEntry["friction"].IsNumber()) {
        friction = componentEntry["friction"].GetFloat();
    }
    
    if (componentEntry.HasMember("bounciness") && componentEntry["bounciness"].IsNumber()) {
        bounciness = componentEntry["bounciness"].GetFloat();
    }
    
    if (componentEntry.HasMember("trigger_type") && componentEntry["trigger_type"].IsString()) {
        trigger_type = componentEntry["trigger_type"].GetString();
    }
    
    if (componentEntry.HasMember("trigger_width") && componentEntry["trigger_width"].IsNumber()) {
        trigger_width = componentEntry["trigger_width"].GetFloat();
    }
    
    if (componentEntry.HasMember("trigger_height") && componentEntry["trigger_height"].IsNumber()) {
        trigger_height = componentEntry["trigger_height"].GetFloat();
    }
    
    if (componentEntry.HasMember("trigger_radius") && componentEntry["trigger_radius"].IsNumber()) {
        trigger_radius = componentEntry["trigger_radius"].GetFloat();
    }
        
    this->actor = actor;
}

RigidBody::RigidBody(Actor* actor) {
    if(!contact_listener) {
        physics_world = new b2World(b2Vec2(0.0f, 9.8f));
        contact_listener = new ContactListener();
        physics_world->SetContactListener(contact_listener);
    }
    
    this->actor = actor;
}


void RigidBody::OnStart() {
    b2BodyDef bodyDef;
    
    if (body_type == "dynamic") {
        bodyDef.type = b2_dynamicBody;
    } else if (body_type == "static") {
        bodyDef.type = b2_staticBody;
    } else if (body_type == "kinematic") {
        bodyDef.type = b2_kinematicBody;
    } else {
        std::cout << "error wrong body type, making it a dynamic type" << std::endl;
        bodyDef.type = b2_dynamicBody;
    }
    
    bodyDef.position.Set(x, y);
    bodyDef.bullet = precise;
    bodyDef.gravityScale = gravity_scale;
    bodyDef.angularDamping = angular_friction;
    
    // Convert degrees to radians for Box2D
    bodyDef.angle = deg_rotation * (b2_pi / 180.0f);
    
    body = physics_world->CreateBody(&bodyDef);
    
    /* phantom sensor to make bodies move if neither collider nor trigger is present */
    if (!has_collider && !has_trigger) {
        b2PolygonShape phantom_shape;
        phantom_shape.SetAsBox(width * 0.5f, height * 0.5f);

        b2FixtureDef phantom_fixture_def;
        phantom_fixture_def.shape = &phantom_shape;
        phantom_fixture_def.density = density;
        phantom_fixture_def.userData.pointer = reinterpret_cast<uintptr_t>(actor); // add the pointer to actor object for things like collisions

        // Because it is a sensor (with no callback even), no collisions will ever occur
        phantom_fixture_def.isSensor = true;

        body->CreateFixture(&phantom_fixture_def);
    } else {
        
        initializeCollider();
        
        initializeTrigger();
    }
}

void RigidBody::OnDestroy() {
    if (body != nullptr) {
        physics_world->DestroyBody(body);
        body = nullptr; // Ensure you don't leave a dangling pointer
    }
}

void RigidBody::initializeCollider() {
    if(!has_collider) {
        return;
    }
    
    b2FixtureDef fixtureDef;
    b2CircleShape circle_shape;
    b2PolygonShape box_shape;

    // now we can make the box
    if(collider_type == "box") {
        box_shape.SetAsBox(width * 0.5f, height * 0.5f); // Placeholder size, you may want to parameterize this
        fixtureDef.shape = &box_shape;
    } else if (collider_type == "circle") {
        circle_shape.m_radius = radius;
        fixtureDef.shape = &circle_shape;
    } else {
        std::cout << "invalid collider_type";
        exit(0);
    }
    
    fixtureDef.filter.categoryBits = CATEGORY_COLLIDER; // It's in the 'trigger' category
    fixtureDef.filter.maskBits = CATEGORY_COLLIDER; // It only collides with other triggers
    fixtureDef.density = density;
    fixtureDef.friction = friction;
    fixtureDef.restitution = bounciness;
    fixtureDef.isSensor = false;
    fixtureDef.userData.pointer = reinterpret_cast<uintptr_t>(actor); // add the pointer to actor object for things like collisions
    body->CreateFixture(&fixtureDef);
}

void RigidBody::initializeTrigger() {
    if(!has_trigger) {
        return;
    }
    
    b2FixtureDef fixtureDef;
    b2CircleShape circle_shape;
    b2PolygonShape box_shape;

    // now we can make the box
    if(trigger_type == "box") {
        box_shape.SetAsBox(trigger_width*0.5f, trigger_height*0.5f); // Placeholder size, you may want to parameterize this
        fixtureDef.shape = &box_shape;
    } else if (trigger_type == "circle") {
        circle_shape.m_radius = trigger_radius;
        fixtureDef.shape = &circle_shape;
    } else {
        std::cout << "invalid trigger_type";
        exit(0);
    }
    
    fixtureDef.filter.categoryBits = CATEGORY_TRIGGER; // It's in the 'trigger' category
    fixtureDef.filter.maskBits = CATEGORY_TRIGGER; // It only collides with other triggers
    fixtureDef.density = density;
    fixtureDef.isSensor = true;
    fixtureDef.userData.pointer = reinterpret_cast<uintptr_t>(actor); // add the pointer to actor object for things like collisions
    body->CreateFixture(&fixtureDef);
}

void RigidBody::physicsStep() {
    if(physics_world) {
        physics_world->Step(1.0f / 60.0f, 8, 3);
    }
}

b2Vec2 RigidBody::GetPosition() { 
    if(!body) {
        return b2Vec2(x, y);
    }
    
    return body->GetPosition();
}

float RigidBody::GetRotation() {
    if(!body) {
        return deg_rotation;
    }
    
    // Convert radians to degrees
    return body->GetAngle() * (180.0f / b2_pi);
}

void RigidBody::AddForce(b2Vec2 force) { 
    if(!body) {
        return;
    }
    
    body->ApplyForceToCenter(force, true);
}

void RigidBody::SetVelocity(b2Vec2 vel) {
    if(!body) {
        return;
    }
    
    body->SetLinearVelocity(vel);
}

void RigidBody::SetPosition(b2Vec2 pos) { 
    if(!body) {
        x = pos.x;
        y = pos.y;
        return;
    }
    
    body->SetTransform(pos, body->GetAngle());
}

void RigidBody::SetRotation(float degrees_clockwise) { 
    if(!body) {
        deg_rotation = degrees_clockwise;
        return;
    }
    
    body->SetTransform(body->GetPosition(), degrees_clockwise * (b2_pi / 180.0f));
}

void RigidBody::SetAngularVelocity(float degrees_clockwise) { 
    if(!body) {
        return;
    }
    
    body->SetAngularVelocity(degrees_clockwise * (b2_pi / 180.0f));
}

void RigidBody::SetGravityScale(float scale) { 
    if(!body) {
        gravity_scale = scale;
    }
    
    body->SetGravityScale(scale);
}

void RigidBody::SetUpDirection(b2Vec2 direction) { 
    direction.Normalize();
    
    // since we want the up direction atan = 0 rad, we have it equal the right vector,
    // atan is typically (y, x) we flip so we put in (x, y)
    float new_angle_rad = glm::atan(direction.x, -direction.y); // direction coming out of top of obj
    if(!body) {
        deg_rotation = new_angle_rad * (180.0f/b2_pi); // convert to degrees
        return;
    }
    
    body->SetTransform(body->GetPosition(), new_angle_rad);
}

void RigidBody::SetRightDirection(b2Vec2 direction) { 
    direction.Normalize();
    float new_angle_rad = glm::atan(direction.x, -direction.y) - b2_pi / 2.0f; // direction coming out of right of obj
    
    if(!body) {
        deg_rotation = new_angle_rad * (180.0f/b2_pi); // convert to degrees
        return;
    }
    
    body->SetTransform(body->GetPosition(), new_angle_rad);;
}

b2Vec2 RigidBody::GetVelocity() { 
    if(!body) {
        return b2Vec2(0,0);
    }
    
    return body->GetLinearVelocity();
}

float RigidBody::GetAngularVelocity() { 
    if(!body) {
        return 0.0f;
    }
    
    return body->GetAngularVelocity() * (180.0f/b2_pi); // convert to degrees
}

float RigidBody::GetGravityScale() { 
    if(!body) {
        return gravity_scale;
    }
    
    return body->GetGravityScale();
}

b2Vec2 RigidBody::GetUpDirection() { 
    if(!body) {
        return b2Vec2(glm::sin(deg_rotation*(b2_pi / 180.0f)), -glm::cos(deg_rotation*(b2_pi / 180.0f)));
    }
    
    // need to flip things so at 0rad = up vector
    b2Vec2 up_dir = b2Vec2(glm::sin(body->GetAngle()), -glm::cos(body->GetAngle()));
    return up_dir;
}

b2Vec2 RigidBody::GetRightDirection() { 
    if(!body) {
        return b2Vec2(glm::cos(deg_rotation*(b2_pi / 180.0f)), glm::sin(deg_rotation*(b2_pi / 180.0f)));
    }
    
    b2Vec2 right_dir = b2Vec2(glm::cos(body->GetAngle()), glm::sin(body->GetAngle())); // rotation measured CLOCKWISE dir
    return right_dir;
}



