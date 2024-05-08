//
//  AudioManager.cpp
//  game_engine
//
//  Created by Mathurin Gagnon on 2/12/24.
//

#include "AudioManager.h"
#include "ComponentDB.h"
#include <iostream>
#include <filesystem>


void AudioManager::initialize() {
    // Initialize SDL_mixer
    if (AudioHelper::Mix_OpenAudio498(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError();
        exit(0);
    }
    
    AudioHelper::Mix_AllocateChannels498(50);
}

void AudioManager::play(int channel, const std::string& sound_name, bool loop) {
    Mix_Chunk* sound = loadSound(sound_name);
    if (sound) {
        AudioHelper::Mix_PlayChannel498(channel, sound, (loop) ? -1 : 0);
    } else {
        std::cout << "error: failed to play audio clip " << sound_name;
        exit(0);
    }
}


Mix_Chunk* AudioManager::loadSound(const std::string& soundName) {
    if (soundMap.find(soundName) != soundMap.end()) {
        return soundMap[soundName]; // Sound already loaded
    }

    // Construct file paths for .wav and .ogg versions
    std::string wavPath = "resources/audio/" + soundName + ".wav";
    std::string oggPath = "resources/audio/" + soundName + ".ogg";

    // Check which file exists and load it
    std::string path = std::filesystem::exists(wavPath) ? wavPath : (std::filesystem::exists(oggPath) ? oggPath : "");
    if (!path.empty()) {
        Mix_Chunk* chunk = AudioHelper::Mix_LoadWAV498(path.c_str());
        if (chunk) {
            soundMap[soundName] = chunk;
            return chunk;
        }
    }

    return nullptr; // Sound could not be loaded
}

void AudioManager::halt(int channel) { 
    AudioHelper::Mix_HaltChannel498(channel); // Halt all channels
}

void AudioManager::setVolume(int channel, float volume) { 
    
    AudioHelper::Mix_Volume498(channel, volume);
}



