//
//  Renderer.cpp
//  dive_engine
//
//  Created by Mathurin Gagnon on 2/7/24.
//

#include <iostream>
#include <filesystem>
#include <cmath>

#include "SDLRenderer.h"
#include "ComponentDB.h"
#include "Helper.h"
#include "EngineUtils.h"
#include "rapidjson/document.h"
#include "SDL2/SDL.h"
#include "SDL_image.h"
#include "glm/glm.hpp"

using std::filesystem::exists, std::string, std::cout;

void SDLRenderer::initialize(const std::string &game_title) {
    
    // defaul display settings
    x_resolution = 640;
    y_resolution = 360;
    clear_color_r = clear_color_g =clear_color_b = 255;
    
    // GET WINDOW CONFIGS
    if(exists("resources/rendering.config")) {
        rapidjson::Document config;
        EngineUtils::ReadJsonFile("resources/rendering.config", config);
        if(config.HasMember("x_resolution")) {
            x_resolution = config["x_resolution"].GetInt();
        }
        
        if(config.HasMember("y_resolution")) {
            y_resolution = config["y_resolution"].GetInt();
        }
        
        if(config.HasMember("clear_color_r")) {
            clear_color_r = config["clear_color_r"].GetInt();
        }
        
        if(config.HasMember("clear_color_g")) {
            clear_color_g = config["clear_color_g"].GetInt();
        }
        
        if(config.HasMember("clear_color_b")) {
            clear_color_b = config["clear_color_b"].GetInt();
        }
        
        if(config.HasMember("zoom_factor")) {
            zoom_factor = config["zoom_factor"].GetFloat();
        }
        
        
    }
    
    
    //INITIALIZE WINDOW
    if(SDL_Init(SDL_INIT_VIDEO)) {
        std::cout << "SDL could not initialize! SDL_ERROR: " << SDL_GetError() << std::endl;
        exit(0);
    }
    
    if (TTF_Init() == -1) {
        std::cerr << "SDL_ttf could not initialize! SDL_ttf Error: " << TTF_GetError() << std::endl;
        exit(0);
    }
    
    window = SDL_CreateWindow(game_title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, x_resolution, y_resolution, SDL_WINDOW_SHOWN);

    if(!window) {
        std::cerr << "Window could not be created! SDL Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return;
    }
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    SDL_RenderSetScale(renderer, zoom_factor, zoom_factor);

}


void SDLRenderer::clearFrame() {
    SDL_SetRenderDrawColor(renderer, clear_color_r, clear_color_g, clear_color_b, 255);
    SDL_RenderClear(renderer);
}

void SDLRenderer::showFrame() {
    SDL_RenderPresent(renderer);
    
    // aim for 60fps
    SDL_Delay();
    frame_number++;
}


SDL_Texture* SDLRenderer::loadImage(const std::string &imgName) {
    auto it = textures.find(imgName);
    
    if(it == textures.end()) {
        if(!exists("resources/images/" + imgName+".png")) {
            cout << "error: missing image " << imgName;
            exit(0);
        }
        
        SDL_Texture* texture = IMG_LoadTexture(renderer, ("resources/images/"+imgName+".png").c_str());
        if (!texture) {
            std::cerr << "Failed to load texture: " << imgName << " SDL_image Error: " << IMG_GetError() << std::endl;
            exit(0);
        } else {
            textures[imgName] = texture;
        }
    }
    
    return textures[imgName];
}


TTF_Font* SDLRenderer::loadFont(const std::string &text_name, const int size) {
    auto it = fonts.find(text_name);
    
    if(it == fonts.end()) {
        if(!exists("resources/fonts/" + text_name+ ".ttf")) {
            cout << "error: missing font " << text_name;
            exit(0);
        }
        
        TTF_Font* font = TTF_OpenFont(("resources/fonts/" + text_name + ".ttf").c_str(), size);
        if (!font) {
            std::cerr << "Failed to load font: " << text_name << " TTF Error: " << TTF_GetError() << std::endl;
            exit(0);
        } else {
            fonts[text_name][size] = font;
        }
    }
    
    return fonts[text_name][size];
}


void SDLRenderer::renderFrame() {
    renderAndClearImgQueue();
    
    renderAndClearUIQueue();
    
    while(text_requests.size()) {
        TextDrawRequest request = text_requests.front();
        
        auto it = fonts.find(request.font_name);
        
        TTF_Font* font = (it != fonts.end() && it->second.find(request.font_size) != it->second.end()) ? it->second[request.font_size] : loadFont(request.font_name, request.font_size);
        
        renderText(font, request);
        
        text_requests.pop();
    }
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    
    for(auto& request : pixel_requests) {
        SDL_SetRenderDrawColor(renderer, request.r, request.g, request.b, request.a);
        SDL_RenderDrawPoint(renderer, request.x, request.y);
    }
    
    pixel_requests.clear();
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

}


void SDLRenderer::renderText(TTF_Font *font, const TextDrawRequest request) {
    
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, request.text.c_str(), {(uint8_t)request.r, (uint8_t)request.g, (uint8_t)request.b, (uint8_t)request.a});
    
    if (!textSurface) {
        std::cerr << "Unable to render text surface! SDL_ttf Error: " << TTF_GetError() << std::endl;
        exit(0);
    }
    
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

    if (!textTexture) {
        std::cerr << "Unable to create texture from rendered text! SDL Error: " << SDL_GetError() << std::endl;
        exit(0);
    }
    
    SDL_RenderSetScale(renderer, 1, 1); // render hud unscaled
    
    SDL_Rect renderQuad = {request.x, request.y, textSurface->w, textSurface->h};
    SDL_FreeSurface(textSurface); // memory leaks

    SDL_RenderCopy(renderer, textTexture, nullptr, &renderQuad);
    SDL_DestroyTexture(textTexture); // memory leaks
    
    SDL_RenderSetScale(renderer, zoom_factor, zoom_factor); // render hud unscaled

}


void SDLRenderer::renderAndClearUIQueue() {
    std::stable_sort(ui_requests.begin(), ui_requests.end(), UIDrawRequest::compare);

    for (auto& request : ui_requests)
    {
        SDL_Texture* tex = loadImage(request.image_name);
        SDL_Rect tex_rect;
        SDL_QueryTexture(tex, NULL, NULL, &tex_rect.w, &tex_rect.h);
        
        //set location on screen, regardless of cam position
        tex_rect.x = request.x;
        tex_rect.y = request.y;

        // Apply tint / alpha to texture
        SDL_SetTextureColorMod(tex, request.r, request.g, request.b);
        SDL_SetTextureAlphaMod(tex, request.a);

        // Perform Draw
        SDL_Point pivot_point = {0, 0};
        SDL_RenderCopyEx(renderer, tex, NULL, &tex_rect, 0, &pivot_point, SDL_FLIP_NONE);

        // Remove tint / alpha from texture
        SDL_SetTextureColorMod(tex, 255, 255, 255);
        SDL_SetTextureAlphaMod(tex, 255);
    }

    ui_requests.clear();
}


void SDLRenderer::renderAndClearImgQueue()
{
    std::stable_sort(img_requests.begin(), img_requests.end(), ImageDrawRequest::compare);
    SDL_RenderSetScale(renderer, zoom_factor, zoom_factor);

    for (auto& request : img_requests)
    {
        
        const int pixels_per_meter = 100;
        glm::vec2 final_rendering_position = glm::vec2(request.x, request.y) - glm::vec2(camera_pos);
        
        SDL_Texture* tex = loadImage(request.image_name);
        SDL_Rect tex_rect;
        SDL_QueryTexture(tex, NULL, NULL, &tex_rect.w, &tex_rect.h);

        // Apply scale
        int flip_mode = SDL_FLIP_NONE;
        if (request.scale_x < 0)
            flip_mode |= SDL_FLIP_HORIZONTAL;
        if (request.scale_y < 0)
            flip_mode |= SDL_FLIP_VERTICAL;

        float x_scale = std::abs(request.scale_x);
        float y_scale = std::abs(request.scale_y);

        tex_rect.w *= x_scale;
        tex_rect.h *= y_scale;

        SDL_Point pivot_point = { static_cast<int>(request.pivot_x * tex_rect.w), static_cast<int>(request.pivot_y * tex_rect.h) };

        glm::ivec2 cam_dimensions = {x_resolution, y_resolution};

        tex_rect.x = static_cast<int>(final_rendering_position.x * pixels_per_meter + cam_dimensions.x * 0.5f * (1.0f / zoom_factor) - pivot_point.x);
        tex_rect.y = static_cast<int>(final_rendering_position.y * pixels_per_meter + cam_dimensions.y * 0.5f * (1.0f / zoom_factor) - pivot_point.y);

        
        // Apply tint / alpha to texture
        SDL_SetTextureColorMod(tex, request.r, request.g, request.b);
        SDL_SetTextureAlphaMod(tex, request.a);

        // Perform Draw
        SDL_RenderCopyEx(renderer, tex, NULL, &tex_rect, request.rotation_degrees, &pivot_point, static_cast<SDL_RendererFlip>(flip_mode));

        SDL_RenderSetScale(renderer, zoom_factor, zoom_factor);

        // Remove tint / alpha from texture
        SDL_SetTextureColorMod(tex, 255, 255, 255);
        SDL_SetTextureAlphaMod(tex, 255);
    }

    SDL_RenderSetScale(renderer, 1, 1);

    img_requests.clear();
}

void SDLRenderer::DrawUI(std::string img_name, float x, float y) {
    ui_requests.push_back({img_name, (int)x, (int)y, 255, 255, 255, 255, 0});
}

void SDLRenderer::DrawUIEx(std::string img_name, float x, float y, float r, float g, float b, float a, float sorting_order) {
    ui_requests.push_back({img_name, (int)x, (int)y, (int)r, (int)g, (int)b, (int)a, (int)sorting_order});
}

void SDLRenderer::DrawImg(std::string img_name, float x, float y) {
    ImageDrawRequest request = {img_name, x, y, 0, 1, 1, 0.5f, 0.5f, 255, 255, 255, 255, 0};

    img_requests.push_back(request);
}

void SDLRenderer::DrawImgEx(std::string img_name, float x, float y, float rotation_degrees, float scale_x, float scale_y, float pivot_x, float pivot_y, float r, float g, float b, float a, float sorting_order) {
    img_requests.push_back({img_name, x, y, (int)rotation_degrees, scale_x, scale_y, pivot_x, pivot_y, (int)r, (int)g, (int)b, (int)a, (int)sorting_order});
}


void SDLRenderer::DrawPixel(float x, float y, float r, float g, float b, float a) {
    pixel_requests.push_back({(int)x, (int)y, (int)r, (int)g, (int)b, (int)a});
}


void SDLRenderer::setCameraPosition(float x, float y) {
//    camera_pos.x = x;
//    camera_pos.y = y;
    camera_pos = glm::vec2(x,y);
}

void SDLRenderer::SDL_Delay() {

    Uint32 current_frame_end_timestamp = SDL_GetTicks();  // Record end time of the frame
    Uint32 current_frame_duration_milliseconds = current_frame_end_timestamp - current_frame_start_timestamp;
    Uint32 desired_frame_duration_milliseconds = 16;

    int delay_ticks = std::max(static_cast<int>(desired_frame_duration_milliseconds) - static_cast<int>(current_frame_duration_milliseconds), 1);

    ::SDL_Delay(delay_ticks);


    current_frame_start_timestamp = SDL_GetTicks();  // Record start time of the frame
}

