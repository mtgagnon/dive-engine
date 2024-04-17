//
//  TemplateData.h
//  game_engine
//
//  Created by Mathurin Gagnon on 2/4/24.
//

#ifndef TEMPLATEDATA_H
#define TEMPLATEDATA_H

#include <memory>
#include <stdio.h>
#include <vector>
#include <unordered_map>
#include <string>
#include "Actor.h"

class TemplateData {
public:
    
    // loads templates from path and returns the template name.
    static void loadTemplate(const std::string& templateName, std::shared_ptr<Actor> child_actor);
    //TemplateData(Actor &base);
    //Actor* getTemplate(const std::string &templateName);
    
private:
    inline static std::vector<std::shared_ptr<Actor>> loadedTemplates;
    inline static std::unordered_map<std::string, std::shared_ptr<Actor>> templateLookup;
    
};


#endif /* TEMPLATEDATA_H */
