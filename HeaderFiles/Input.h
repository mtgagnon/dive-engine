//
//  Input.h
//  game_engine
//
//  Created by Mathurin Gagnon on 2/15/24.
//

#ifndef INPUT_H
#define INPUT_H

#include "SDL2/SDL.h"
#include "SDL2/SDL_gamecontroller.h"

#include "glm/glm.hpp"

#include <unordered_map>
#include <map>
#include <vector>
#include <string>

enum INPUT_STATE {
    INPUT_STATE_UP,
    INPUT_STATE_JUST_BECAME_DOWN,
    INPUT_STATE_DOWN,
    INPUT_STATE_JUST_BECAME_UP
};

struct Controller {
    
};

class Input {
public:
    static void initialize(); // Call before main loop begins.
    static void ProcessEvent(const SDL_Event & e); // Call every frame at start of event loop.
    static void LateUpdate();
    static void shutdown();
    
    // Lua exposed functions
    static bool GetKey(std::string keycode);
    static bool GetKeyDown(std::string keycode);
    static bool GetKeyUp(std::string keycode);
    
    static bool GetControllerButton(int controller_id, std::string keycode);
    static bool GetControllerButtonDown(int controller_id, std::string keycode);
    static bool GetControllerButtonUp(int controller_id, std::string keycode);
    static float GetControllerAxis(int controller_id, std::string keycode);
    
    static glm::vec2 GetMousePosition();

    static bool GetMouseButton(int button);
    static bool GetMouseButtonDown(int button);
    static bool GetMouseButtonUp(int button);
    static float GetMouseScrollDelta();
        
    static void AddController(int index);
    static void RemoveController(int index);
    
    static SDL_Scancode GetSDLScancodeFromLuaKey(const std::string& lua_key);
    static SDL_GameControllerButton GetControllerButtonFromLuaKey(const std::string& lua_key);
    static SDL_GameControllerAxis GetControllerAxisFromLuaKey(const std::string& lua_key);

private:
    
    static inline std::unordered_map<SDL_Scancode, INPUT_STATE> keyboard_states;
    static inline std::vector<SDL_Scancode> just_became_down_scancodes;
    static inline std::vector<SDL_Scancode> just_became_up_scancodes;

    static inline std::map<int, SDL_GameController*> controllers;  // Handles to open controllers
    static inline std::unordered_map<int, std::unordered_map<int, INPUT_STATE>> controller_button_states; // controller id to (SDL_GameControllerButton, input_state)
    static inline std::unordered_map<int, std::unordered_map<int, float>> controller_axis_states; // controller id to (SDL_GameControllerAxis, float)
    static inline std::vector<std::pair<int, int>> controllers_just_down_buttons; // controller id to button
    static inline std::vector<std::pair<int, int>> controllers_just_up_buttons; // controller id to button
    
    static inline glm::vec2 mouse_position;
    static inline std::unordered_map<int, INPUT_STATE> mouse_button_states;
    static inline std::vector<int> just_down_buttons;
    static inline std::vector<int> just_up_buttons;
    
    static inline std::unordered_map<std::string, SDL_GameControllerButton> __button_to_controller_button = {
        {"a", SDL_CONTROLLER_BUTTON_A},
        {"b", SDL_CONTROLLER_BUTTON_B},
        {"x", SDL_CONTROLLER_BUTTON_X},
        {"y", SDL_CONTROLLER_BUTTON_Y},
        {"back", SDL_CONTROLLER_BUTTON_BACK},
        {"guide", SDL_CONTROLLER_BUTTON_GUIDE},
        {"start", SDL_CONTROLLER_BUTTON_START},
        {"leftstick", SDL_CONTROLLER_BUTTON_LEFTSTICK},
        {"rightstick", SDL_CONTROLLER_BUTTON_RIGHTSTICK},
        {"leftshoulder", SDL_CONTROLLER_BUTTON_LEFTSHOULDER},
        {"rightshoulder", SDL_CONTROLLER_BUTTON_RIGHTSHOULDER},
        {"dpad_up", SDL_CONTROLLER_BUTTON_DPAD_UP},
        {"dpad_down", SDL_CONTROLLER_BUTTON_DPAD_DOWN},
        {"dpad_left", SDL_CONTROLLER_BUTTON_DPAD_LEFT},
        {"dpad_right", SDL_CONTROLLER_BUTTON_DPAD_RIGHT}
    };

    static inline std::unordered_map<std::string, SDL_GameControllerAxis> __axis_to_controller_axis = {
        {"leftx", SDL_CONTROLLER_AXIS_LEFTY},
        {"lefty", SDL_CONTROLLER_AXIS_LEFTX},
        {"rightx", SDL_CONTROLLER_AXIS_RIGHTX},
        {"righty", SDL_CONTROLLER_AXIS_RIGHTY},
        {"triggerleft", SDL_CONTROLLER_AXIS_TRIGGERLEFT},
        {"triggerright", SDL_CONTROLLER_AXIS_TRIGGERRIGHT}
    };
    
    const static inline std::unordered_map<std::string, SDL_Scancode> __keycode_to_scancode = {
        // Directional (arrow) Keys
        {"up", SDL_SCANCODE_UP},
        {"down", SDL_SCANCODE_DOWN},
        {"right", SDL_SCANCODE_RIGHT},
        {"left", SDL_SCANCODE_LEFT},

        // Misc Keys
        {"escape", SDL_SCANCODE_ESCAPE},

        // Modifier Keys
        {"lshift", SDL_SCANCODE_LSHIFT},
        {"rshift", SDL_SCANCODE_RSHIFT},
        {"lctrl", SDL_SCANCODE_LCTRL},
        {"rctrl", SDL_SCANCODE_RCTRL},
        {"lalt", SDL_SCANCODE_LALT},
        {"ralt", SDL_SCANCODE_RALT},

        // Editing Keys
        {"tab", SDL_SCANCODE_TAB},
        {"return", SDL_SCANCODE_RETURN},
        {"enter", SDL_SCANCODE_RETURN},
        {"backspace", SDL_SCANCODE_BACKSPACE},
        {"delete", SDL_SCANCODE_DELETE},
        {"insert", SDL_SCANCODE_INSERT},

        // Character Keys
        {"space", SDL_SCANCODE_SPACE},
        {"a", SDL_SCANCODE_A},
        {"b", SDL_SCANCODE_B},
        {"c", SDL_SCANCODE_C},
        {"d", SDL_SCANCODE_D},
        {"e", SDL_SCANCODE_E},
        {"f", SDL_SCANCODE_F},
        {"g", SDL_SCANCODE_G},
        {"h", SDL_SCANCODE_H},
        {"i", SDL_SCANCODE_I},
        {"j", SDL_SCANCODE_J},
        {"k", SDL_SCANCODE_K},
        {"l", SDL_SCANCODE_L},
        {"m", SDL_SCANCODE_M},
        {"n", SDL_SCANCODE_N},
        {"o", SDL_SCANCODE_O},
        {"p", SDL_SCANCODE_P},
        {"q", SDL_SCANCODE_Q},
        {"r", SDL_SCANCODE_R},
        {"s", SDL_SCANCODE_S},
        {"t", SDL_SCANCODE_T},
        {"u", SDL_SCANCODE_U},
        {"v", SDL_SCANCODE_V},
        {"w", SDL_SCANCODE_W},
        {"x", SDL_SCANCODE_X},
        {"y", SDL_SCANCODE_Y},
        {"z", SDL_SCANCODE_Z},
        {"0", SDL_SCANCODE_0},
        {"1", SDL_SCANCODE_1},
        {"2", SDL_SCANCODE_2},
        {"3", SDL_SCANCODE_3},
        {"4", SDL_SCANCODE_4},
        {"5", SDL_SCANCODE_5},
        {"6", SDL_SCANCODE_6},
        {"7", SDL_SCANCODE_7},
        {"8", SDL_SCANCODE_8},
        {"9", SDL_SCANCODE_9},
        {"/", SDL_SCANCODE_SLASH},
        {";", SDL_SCANCODE_SEMICOLON},
        {"=", SDL_SCANCODE_EQUALS},
        {"-", SDL_SCANCODE_MINUS},
        {".", SDL_SCANCODE_PERIOD},
        {",", SDL_SCANCODE_COMMA},
        {"[", SDL_SCANCODE_LEFTBRACKET},
        {"]", SDL_SCANCODE_RIGHTBRACKET},
        {"\\", SDL_SCANCODE_BACKSLASH},
        {"'", SDL_SCANCODE_APOSTROPHE}
    };

    static inline float mouse_scroll_this_frame = 0;
};
#endif
