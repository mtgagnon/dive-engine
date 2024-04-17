//
//  EventBus.h
//  game_engine
//
//  Created by Mathurin Gagnon on 3/28/24.
//

#ifndef EVENTBUS_H
#define EVENTBUS_H

#include <stdio.h>
#include <string>
#include <map>
#include <list>

#include "lua.hpp"
#include "LuaBridge.h"
#include "ComponentDB.h"

#endif /* EVENTBUS_H */

struct Subscription {
    Subscription(luabridge::LuaRef component, luabridge::LuaRef callback) : component(component), callback(callback){}
    luabridge::LuaRef component = luabridge::LuaRef(ComponentDB::GetLuaState()); // set to nil
    luabridge::LuaRef callback = luabridge::LuaRef(ComponentDB::GetLuaState()); // set to nil
};

class EventBus {
public:
    static void Publish(std::string event_type, luabridge::LuaRef event_object);
    static void Subscribe(std::string event_type, luabridge::LuaRef component, luabridge::LuaRef function);
    static void Unsubscribe(std::string event_type, luabridge::LuaRef component, luabridge::LuaRef function);
    
    static void PushChangesToSubList();
    
    static inline std::map<std::string, std::list<std::list<Subscription>::iterator>> event_subs_to_unsub; // map to list of iterators
    static inline std::map<std::string, std::list<Subscription>> event_subs_to_add;
    static inline std::map<std::string, std::list<Subscription>> event_subs;
};
