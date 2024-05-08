//
//  Input.cpp
//  game_engine
//
//  Created by Mathurin Gagnon on 2/15/24.
//

#include "Input.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_gamecontroller.h"

#include <cmath>

#include "ComponentDB.h"
#include <iostream>


void Input::initialize() {
    SDL_Init(SDL_INIT_GAMECONTROLLER);
}

void Input::shutdown() {
    for (auto controller : controllers) {
        if (controller.second != nullptr) {
            SDL_GameControllerClose(controller.second);
        }
    }
    
    controllers.clear();  // Clear the list of controllers
}

void Input::ProcessEvent(const SDL_Event &e) {
    switch (e.type) {
        case SDL_MOUSEMOTION:
            mouse_position.x = e.motion.x;
            mouse_position.y = e.motion.y;
            break;
        case SDL_MOUSEWHEEL:
            mouse_scroll_this_frame += e.wheel.preciseY;
            break;
        case SDL_MOUSEBUTTONUP:
            mouse_button_states[e.button.button] = INPUT_STATE_JUST_BECAME_UP;
            just_up_buttons.push_back(e.button.button);
            break;
        case SDL_MOUSEBUTTONDOWN:
            mouse_button_states[e.button.button] = INPUT_STATE_JUST_BECAME_DOWN;
            just_down_buttons.push_back(e.button.button);
            break;
        case SDL_KEYDOWN:
            keyboard_states[e.key.keysym.scancode] = INPUT_STATE_JUST_BECAME_DOWN;
            just_became_down_scancodes.push_back(e.key.keysym.scancode);
            break;
        case SDL_KEYUP:
            keyboard_states[e.key.keysym.scancode] = INPUT_STATE_JUST_BECAME_UP;
            just_became_up_scancodes.push_back(e.key.keysym.scancode);
            break;
        case SDL_CONTROLLERBUTTONDOWN:
            controller_button_states[e.cdevice.which][e.cbutton.button] = INPUT_STATE_JUST_BECAME_DOWN;
            controllers_just_down_buttons.push_back({e.cdevice.which, e.cbutton.button});
            break;
        case SDL_CONTROLLERBUTTONUP:
            controller_button_states[e.cdevice.which][e.cbutton.button] = INPUT_STATE_JUST_BECAME_UP;
            controllers_just_up_buttons.push_back({e.cdevice.which, e.cbutton.button});
            break;
        case SDL_CONTROLLERAXISMOTION:
            controller_axis_states[e.cdevice.which][e.caxis.axis] = e.caxis.value;
            break;
        case SDL_CONTROLLERDEVICEADDED:
            AddController(e.cdevice.which);
            break;
        case SDL_CONTROLLERDEVICEREMOVED:
            RemoveController(e.cdevice.which);
            break;
    }
}

void Input::LateUpdate() {
    for(auto key : just_became_down_scancodes) {
        keyboard_states[key] = (keyboard_states[key] == INPUT_STATE_JUST_BECAME_DOWN) ? INPUT_STATE_DOWN : keyboard_states[key];
    }
    
    for(auto key : just_became_up_scancodes) {
        keyboard_states[key] = (keyboard_states[key] == INPUT_STATE_JUST_BECAME_UP) ? INPUT_STATE_UP : keyboard_states[key];
    }
    
    for(auto button : just_down_buttons) {
        mouse_button_states[button] = (mouse_button_states[button] == INPUT_STATE_JUST_BECAME_DOWN) ? INPUT_STATE_DOWN : mouse_button_states[button];
    }
    
    for(auto button : just_up_buttons) {
        mouse_button_states[button] = (mouse_button_states[button] == INPUT_STATE_JUST_BECAME_UP) ? INPUT_STATE_UP : mouse_button_states[button];
    }
    
    for(auto pair : controllers_just_down_buttons) {
        int id = pair.first;
        int button = pair.second;
        INPUT_STATE state = controller_button_states[id][button];
        
        controller_button_states[id][button] = (state == INPUT_STATE_JUST_BECAME_DOWN) ? INPUT_STATE_DOWN : state;
    }
    
    for(auto pair : controllers_just_up_buttons) {
        int id = pair.first;
        int button = pair.second;
        INPUT_STATE state = controller_button_states[id][button];
        
        controller_button_states[id][button] = (state == INPUT_STATE_JUST_BECAME_UP) ? INPUT_STATE_UP : state;
    }
    
    mouse_scroll_this_frame = 0;
    
    just_up_buttons.clear();
    just_down_buttons.clear();
    just_became_up_scancodes.clear();
    just_became_down_scancodes.clear();
    controllers_just_up_buttons.clear();
    controllers_just_down_buttons.clear();
}


//------------------------ CONTROLLERS BUTTONS ---------------------------//
bool Input::GetControllerButton(int controller_id, std::string keycode) {
    auto it = controller_button_states.find(controller_id);
    
    if(it != controller_button_states.end()) {
        int button = GetControllerButtonFromLuaKey(keycode);
        auto button_state = it->second.find(button);
        
        return button_state != it->second.end() && (button_state->second == INPUT_STATE_JUST_BECAME_DOWN || button_state->second == INPUT_STATE_DOWN);
    } else {
        return false;
    }
}

bool Input::GetControllerButtonDown(int controller_id, std::string keycode) {
    auto it = controller_button_states.find(controller_id);
    
    if(it != controller_button_states.end()) {
        int button = GetControllerButtonFromLuaKey(keycode);
        auto button_state = it->second.find(button);
        
        return button_state != it->second.end() && (button_state->second == INPUT_STATE_JUST_BECAME_DOWN);
    } else {
        return false;
    }
}

bool Input::GetControllerButtonUp(int controller_id, std::string keycode) {
    auto it = controller_button_states.find(controller_id);
    
    if(it != controller_button_states.end()) {
        int button = GetControllerButtonFromLuaKey(keycode);
        auto button_state = it->second.find(button);
        
        return button_state != it->second.end() && (button_state->second == INPUT_STATE_JUST_BECAME_UP);
    } else {
        return false;
    }
}

float Input::GetControllerAxis(int controller_id, std::string axisName) {
    auto controller = controller_axis_states.find(controller_id);
    
    if(controller != controller_axis_states.end()) {
        SDL_GameControllerAxis axis = GetControllerAxisFromLuaKey(axisName);
        auto it = controller->second.find(axis);
        
        float input = 0.0f;
        if(axis == SDL_CONTROLLER_AXIS_LEFTY || axis == SDL_CONTROLLER_AXIS_RIGHTY) {
            input = (it == controller->second.end()) ? 0.0f : std::round(-it->second/32768.0f * 1000.0f)/1000.f;
        } else {
            input = (it == controller->second.end()) ? 0.0f : std::round(it->second/32768.0f * 1000.0f)/1000.f;
        }
        
        input = (input < 0.3f && input > -0.3f) ? 0.0f : input;
        
        return input;
        
    } else {
        return 0.0f;
    }
}



//----------------------- NORMAL BUTTONS ---------------------------
bool Input::GetKey(std::string keycode) {
    auto it = keyboard_states.find(GetSDLScancodeFromLuaKey(keycode));
    return it != keyboard_states.end() && (it->second == INPUT_STATE_JUST_BECAME_DOWN || it->second == INPUT_STATE_DOWN);
}

bool Input::GetKeyDown(std::string keycode) {
    auto it = keyboard_states.find(GetSDLScancodeFromLuaKey(keycode));
    return it != keyboard_states.end() && it->second == INPUT_STATE_JUST_BECAME_DOWN;
}

bool Input::GetKeyUp(std::string keycode) {
    auto it = keyboard_states.find(GetSDLScancodeFromLuaKey(keycode));
    return it != keyboard_states.end() && it->second == INPUT_STATE_JUST_BECAME_UP;
}

glm::vec2 Input::GetMousePosition() { 
    return mouse_position;
}

bool Input::GetMouseButton(int button) {
    auto it = mouse_button_states.find(button);
    return it != mouse_button_states.end() && (it->second == INPUT_STATE_JUST_BECAME_DOWN || it->second == INPUT_STATE_DOWN);
}

bool Input::GetMouseButtonDown(int button) { 
    auto it = mouse_button_states.find(button);
    return it != mouse_button_states.end() && it->second == INPUT_STATE_JUST_BECAME_DOWN;
}

bool Input::GetMouseButtonUp(int button) {
    auto it = mouse_button_states.find(button);
    return it != mouse_button_states.end() && it->second == INPUT_STATE_JUST_BECAME_UP;
}

float Input::GetMouseScrollDelta() { 
    return mouse_scroll_this_frame;
}

SDL_GameControllerButton Input::GetControllerButtonFromLuaKey(const std::string& lua_key) {
    auto it = __button_to_controller_button.find(lua_key);
    if (it != __button_to_controller_button.end()) {
        return it->second;
    }
    
    // Return some invalid scancode or handle error appropriately
    return SDL_CONTROLLER_BUTTON_MISC1;
}

SDL_GameControllerAxis Input::GetControllerAxisFromLuaKey(const std::string& lua_key) {
    auto it = __axis_to_controller_axis.find(lua_key);
    if (it != __axis_to_controller_axis.end()) {
        return it->second;
    }
    
    // Return some invalid scancode or handle error appropriately
    return SDL_CONTROLLER_AXIS_INVALID;
}

SDL_Scancode Input::GetSDLScancodeFromLuaKey(const std::string& lua_key) {
    auto it = __keycode_to_scancode.find(lua_key);
    if (it != __keycode_to_scancode.end()) {
        return it->second;
    }
    
    // Return some invalid scancode or handle error appropriately
    return SDL_SCANCODE_UNKNOWN;
}

void Input::AddController(int index) {
    if (!SDL_IsGameController(index)) {
        return;
    }
    
    SDL_GameController* controller = SDL_GameControllerOpen(index);
    if(controller) { // don't add controller if it isn't correct
        controllers[index] = controller;
    } else {
        std::cout << "failed ot add controller: " << index << std::endl;
    }
}

void Input::RemoveController(int index) {
    auto it = controllers.find(index);
    if(it != controllers.end()) {
        controllers.erase(it);
    }
}
