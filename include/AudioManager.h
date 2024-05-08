//
//  AudioManager.h
//  game_engine
//
//  Created by Mathurin Gagnon on 2/12/24.
//

#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <string>
#include <unordered_map>
#include "AudioHelper.h"


class AudioManager {
public:
    static void initialize();
    
    static void play(int channel, const std::string& sound_name, bool loop);
    
    // void stopSound(const std::string& soundName); // Stop a specific sound
    
    static void halt(int channel);
    
    static void setVolume(int channel, float volume);

private:
    static inline std::unordered_map<std::string, Mix_Chunk*> soundMap; // Maps sound names to their Mix_Chunk pointers
    static inline Mix_Chunk* loadSound(const std::string& soundName); // Utility function to load a sound
};

#endif /* AUDIOMANAGER_H */
