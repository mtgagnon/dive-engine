#ifndef AUDIOHELPER_H
#define AUDIOHELPER_H

#define AUDIO_HELPER_VERSION 0.7

#include <iostream>
#include <filesystem>

/* WARNING : You may need to adjust the following include paths if your headers / file structures are different. */
/* Here is the instructor solution folder structure (if we make $(ProjectDir) a include directory, these paths are valid. */
/* https://bit.ly/3OClfHc */
#include "SDL2_mixer/SDL_mixer.h"
#include "Helper.h"

class AudioHelper {
    public:
        static inline Mix_Chunk* Mix_LoadWAV498(const char* file)
        {
            if (!IsAutograderMode())
                return Mix_LoadWAV(file);
            else
            {
                if (std::filesystem::exists(file))
                    return &autograder_dummy_sound;
                else
                    return nullptr;
            }
        }
        
        static inline int Mix_PlayChannel498(int channel, Mix_Chunk *chunk, int loops)
        {
            std::cout << "(Mix_PlayChannel498(" << channel << ",?," << loops << ") called on frame " << Helper::GetFrameNumber() << ")" << std::endl;
            
            if (!IsAutograderMode())
                return Mix_PlayChannel(channel, chunk, loops);
            else
            {
                return channel;
            }
        }

        static inline int Mix_OpenAudio498(int frequency, Uint16 format, int channels, int chunksize)
        {
            if (!IsAutograderMode())
                return Mix_OpenAudio(frequency, format, channels, chunksize);
            else
                return 0;
        }

        static inline int Mix_AllocateChannels498(int numchans)
        {
            if (!IsAutograderMode())
                return Mix_AllocateChannels(numchans);
            else
                return numchans;
        }
                
        static inline void Mix_Pause498(int channel)
        {
            std::cout << "(Mix_Pause498(" << channel << ") called on frame " << Helper::GetFrameNumber() << ")" << std::endl;
            
            if (!IsAutograderMode())
                Mix_Pause(channel);
        }
        static inline void Mix_Resume498(int channel)
        {
            std::cout << "(Mix_Resume498(" << channel << ") called on frame " << Helper::GetFrameNumber() << ")" << std::endl;
            
            if (!IsAutograderMode())
                Mix_Resume(channel);
        }
        
        static inline int Mix_HaltChannel498(int channel)
        {
            std::cout << "(Mix_HaltChannel498(" << channel << ") called on frame " << Helper::GetFrameNumber() << ")" << std::endl;
            
            if (!IsAutograderMode())
                return Mix_HaltChannel(channel);
            else
                return 0;
        }
        
        static inline int Mix_Volume498(int channel, int volume)
        {
            std::cout << "(Mix_Volume498(" << channel << "," << volume << ") called on frame " << Helper::GetFrameNumber() << ")" << std::endl;
            
            if (!IsAutograderMode())
                return Mix_Volume(channel, volume);
            else
                return 0;
        }
        
        static inline void Mix_CloseAudio498(void)
        {
            if (!IsAutograderMode())
                Mix_CloseAudio();
        }
    
    private:
        static inline Mix_Chunk autograder_dummy_sound;

        /* It's best for everyone if the autograder doesn't actually play any audio (it will still check that you try though). */
        /* Imagine poor Professor Kloosterman hearing the autograder all day as it spams various sounds in my office. */
        static inline bool IsAutograderMode() {
            /* Visual C++ does not like std::getenv */
#ifdef _WIN32
            char* val = nullptr;
            size_t length = 0;
            _dupenv_s(&val, &length, "AUTOGRADER");
            if (val) {
                free(val);
                return true;
            }
            else {
                return false;
            }
#else
            const char* autograder_mode_env_variable = std::getenv("AUTOGRADER");
            if (autograder_mode_env_variable)
                return true;
            else
                return false;
#endif

            return false;
        }
};

/* Prevent students from using the default Mix-related functions. */
#define Mix_LoadWAV Please_use_Mix_LoadWAV498_instead
#define Mix_OpenAudio Please_use_Mix_OpenAudio498_instead
#define Mix_AllocateChannels Please_use_Mix_AllocateChannels498_instead
#define Mix_PlayChannel Please_use_Mix_PlayChannel498_instead
#define Mix_Pause Please_use_Mix_Pause498_instead
#define Mix_Resume Please_use_Mix_Resume498_instead
#define Mix_HaltChannel Please_use_Mix_HaltChannel498_instead
#define Mix_Volume Please_use_Mix_Volume498_instead
#define Mx_CloseAudio Please_use_Mix_CloseAudio498_instead

#define Mix_LoadMUS function_not_allowed_on_assignment
#define Mix_PlayMusic function_not_allowed_on_assignment
#define Mix_VolumeMusic function_not_allowed_on_assignment
#define Mix_HaltMusic function_not_allowed_on_assignment
#define Mix_PauseMusic function_not_allowed_on_assignment
#define Mix_ResumeMusic function_not_allowed_on_assignment

#endif
