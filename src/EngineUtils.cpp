//
//  EngineUtils.h
//  game_engine
//
//  Created by Mathurin Gagnon on 2/1/24.
//

#include <sstream>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <algorithm>
#include <optional>

#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "EngineUtils.h"
#include "Actor.h"

using std::string, std::cout;

void EngineUtils::ReadJsonFile(const std::string& path, rapidjson::Document & out_document)
{
    FILE* file_pointer = nullptr;
#ifdef _WIN32
    fopen_s(&file_pointer, path.c_str(), "rb");
#else
    file_pointer = fopen(path.c_str(), "rb");
#endif
    char buffer[65536];
    rapidjson::FileReadStream stream(file_pointer, buffer, sizeof(buffer));
    out_document.ParseStream(stream);
    std::fclose(file_pointer);

    if (out_document.HasParseError()) {
        // rapidjson::ParseErrorCode errorCode = out_document.GetParseError();
        std::cout << "error parsing json at [" << path << "]" << std::endl;
        exit(0);
    }
}

uint64_t EngineUtils::GetXYNum(int x, int y) {
    uint64_t num = static_cast<uint32_t>(x);
    num = num << 32;
    num = num | static_cast<uint32_t>(y);
    return num;
}

void EngineUtils::printActor(Actor& actor) {
    cout << "name: " << actor.name << "\n";


}

std::string EngineUtils::obtain_word_after_phrase(const std::string& input, const std::string& phrase) {
    // Find the position of the phrase in the string
    size_t pos = input.find(phrase);

    // If phrase is not found, return an empty string
    if (pos == std::string::npos) return "";

    // Find the starting position of the next word (skip spaces after the phrase)
    pos += phrase.length();
    while (pos < input.size() && std::isspace(input[pos])) {
        ++pos;
    }

    // If we're at the end of the string, return an empty string
    if (pos == input.size()) return "";

    // Find the end position of the word (until a space or the end of the string)
    size_t endPos = pos;
    while (endPos < input.size() && !std::isspace(input[endPos])) {
        ++endPos;
    }

    // Extract and return the word
    return input.substr(pos, endPos - pos);
}


void EngineUtils::ReportError(const std::string& actor_name, const luabridge::LuaException& e) {
    std::string error_message = e.what();

    /* Normalize file paths across platforms */
    std::replace(error_message.begin(), error_message.end(), '\\', '/');

    /* Display (with color codes) */
    std::cout << "\033[31m" << actor_name << " : " << error_message << "\033[0m" << std::endl;
}
