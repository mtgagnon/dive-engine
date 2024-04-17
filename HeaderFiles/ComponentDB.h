//
//  ComponentDB.h
//  game_engine
//
//  Created by Mathurin Gagnon on 3/8/24.
//

#ifndef COMPONENTDB_H
#define COMPONENTDB_H

#include <stdio.h>
#include <unordered_set>
#include <string>

#include "lua.hpp"
#include "LuaBridge.h"

class ComponentDB {
public:
    
    static void initComponentDB();
    
    static lua_State* GetLuaState() {return lua_state;}
    
    static luabridge::Namespace getLuaInstanceGlobalNameSpace() {return luabridge::getGlobalNamespace(lua_state);}
    
    static luabridge::LuaRef getNil();
    
    static luabridge::LuaRef getParentRef(std::string &type);

    static luabridge::LuaRef getEmptyTable() {return luabridge::newTable(lua_state);}

    static void establishLuaInheritance(luabridge::LuaRef& instance_table, luabridge::LuaRef& parent_table);
        
private:
        
    // Debug funcs
    static void CppLog(const std::string& message);
    static void CppLogError(const std::string& message);

    // application namespace funcs
    static void quit();
    static void sleep(int milliseconds);
    static int getFrame();
    static void openURL(std::string url);
    
    static inline lua_State* lua_state = nullptr;
    static inline std::unordered_set<std::string> components;
};

#endif /* COMPONENTDB_H */
