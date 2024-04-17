//
//  EventBus.cpp
//  game_engine
//
//  Created by Mathurin Gagnon on 3/28/24.
//  Current implementation has it if you don't us=nsubscribe a component and then destroy it,
//  could cause undefined behavior

#include <list>
#include "EventBus.h"
#include "Actor.h"


void EventBus::Publish(std::string event_type, luabridge::LuaRef event_object) {
    
    auto list_it = event_subs.find(event_type);
        if (list_it != event_subs.end()) {
            auto& sub_list = list_it->second;

            for (auto it = sub_list.begin(); it != sub_list.end(); ) {
                auto& sub = *it;
                if (sub.callback.isFunction() && sub.component["enabled"].isBool() && sub.component["enabled"].cast<bool>()) {
                    sub.callback(sub.component, event_object);
                    it++;
                } else {
                    it++;
                }
            }
        }
}

void EventBus::Subscribe(std::string event_type, luabridge::LuaRef component, luabridge::LuaRef function) {
    event_subs_to_add[event_type].emplace_back(component, function); // add to the sub list
}

void EventBus::Unsubscribe(std::string event_type, luabridge::LuaRef component, luabridge::LuaRef function) { 
    auto list_it = event_subs.find(event_type);
    
    if(list_it != event_subs.end()) {
        std::list<Subscription>& sub_list = list_it->second;

        // go through subs, to remove component, remove abandoned ones as well
        for(auto it = sub_list.begin(); it != sub_list.end(); it++) {
            auto sub = *it;
            if(!sub.component.isTable() || sub.component.isNil() || (sub.component == component && sub.callback == function)) {
                event_subs_to_unsub[event_type].push_back(it);
            }
        }
    }
}

void EventBus::PushChangesToSubList() {
    for(auto event_pair : event_subs_to_add) {
        for(auto sub : event_pair.second) {
            event_subs[event_pair.first].push_back(sub);
        }
    }
    
    for(auto unsub_list : event_subs_to_unsub) {
        std::string event_type = unsub_list.first;
        std::list subscription_list = event_subs[event_type];
        for(auto sub_to_remove : unsub_list.second) {
            subscription_list.erase(sub_to_remove);
        }
    }
    
    event_subs_to_add.clear();
    event_subs_to_unsub.clear();
}
