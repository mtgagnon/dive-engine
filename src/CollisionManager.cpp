//
//  CollisionManager.cpp
//  game_engine
//
//  Created by Mathurin Gagnon on 3/26/24.
//

#include "Rigidbody.h"
#include "CollisionManager.h"
#include "box2d/box2d.h"

void ContactListener::BeginContact(b2Contact *contact) {
    b2Fixture* fixture_a = contact->GetFixtureA();
    b2Fixture* fixture_b = contact->GetFixtureB();
    
    // do a and then b
    Actor* actor_a = reinterpret_cast<Actor*>(fixture_a->GetUserData().pointer);
    Actor* actor_b = reinterpret_cast<Actor*>(fixture_b->GetUserData().pointer);
    
    b2WorldManifold manifold;
    contact->GetWorldManifold(&manifold);
    
    b2Body* body_a = fixture_a->GetBody();
    b2Body* body_b = fixture_b->GetBody();
    b2Vec2 relative_vel = body_a->GetLinearVelocity() - body_b->GetLinearVelocity();
    
    if(fixture_a->GetFilterData().categoryBits == CATEGORY_COLLIDER && fixture_b->GetFilterData().categoryBits == CATEGORY_COLLIDER) { // they are colliders
        actor_a->OnCollisionEnter({actor_b, manifold.points[0], relative_vel, manifold.normal});
        actor_b->OnCollisionEnter({actor_a, manifold.points[0], relative_vel, manifold.normal});
    } else if(fixture_a->GetFilterData().categoryBits == CATEGORY_TRIGGER && fixture_b->GetFilterData().categoryBits == CATEGORY_TRIGGER) {
        actor_a->OnTriggerEnter({actor_b, b2Vec2(-999.0f,-999.0f), relative_vel, b2Vec2(-999.0f,-999.0f)});
        actor_b->OnTriggerEnter({actor_a, b2Vec2(-999.0f,-999.0f), relative_vel, b2Vec2(-999.0f,-999.0f)});
    }

}

void ContactListener::EndContact(b2Contact *contact) {
    b2Fixture* fixture_a = contact->GetFixtureA();
    b2Fixture* fixture_b = contact->GetFixtureB();
    
    // do a and then b
    Actor* actor_a = reinterpret_cast<Actor*>(fixture_a->GetUserData().pointer);
    Actor* actor_b = reinterpret_cast<Actor*>(fixture_b->GetUserData().pointer);
    
    b2Body* body_a = fixture_a->GetBody();
    b2Body* body_b = fixture_b->GetBody();
    b2Vec2 relative_vel = body_a->GetLinearVelocity() - body_b->GetLinearVelocity();
    
    if(fixture_a->GetFilterData().categoryBits == CATEGORY_COLLIDER && fixture_b->GetFilterData().categoryBits == CATEGORY_COLLIDER) { // they are colliders
        actor_a->OnCollisionExit({actor_b, b2Vec2(-999.0f,-999.0f), relative_vel, b2Vec2(-999.0f,-999.0f)});
        actor_b->OnCollisionExit({actor_a, b2Vec2(-999.0f,-999.0f), relative_vel, b2Vec2(-999.0f,-999.0f)});
    } else if(fixture_a->GetFilterData().categoryBits == CATEGORY_TRIGGER && fixture_b->GetFilterData().categoryBits == CATEGORY_TRIGGER) {
        actor_a->OnTriggerExit({actor_b, b2Vec2(-999.0f,-999.0f), relative_vel, b2Vec2(-999.0f,-999.0f)});
        actor_b->OnTriggerExit({actor_a, b2Vec2(-999.0f,-999.0f), relative_vel, b2Vec2(-999.0f,-999.0f)});
    }
}
