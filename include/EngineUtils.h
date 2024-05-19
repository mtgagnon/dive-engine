//
//  EngineUtils.h
//  game_engine
//
//  Created by Mathurin Gagnon on 1/31/24.
//

#ifndef ENGINEUTILS_H
#define ENGINEUTILS_H

#include <string>
#include <vector>

#include "Actor.h"
#include "TemplateData.h"
#include "rapidjson/document.h"

class EngineUtils {
public:
    
    static void ReadJsonFile(const std::string& path, rapidjson::Document & out_document);
    
    static void ReportError(const std::string& actor_name, const luabridge::LuaException& e);
    
    static uint64_t GetXYNum(int x, int y);
    
    static void printActor(Actor& actor);
    
    static std::string obtain_word_after_phrase(const std::string& input, const std::string& phrase);
    
};

#endif /* ENGINEUTILS_H */
