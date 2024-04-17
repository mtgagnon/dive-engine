//
//  Renderer.h
//  game_engine
//
//  Created by Mathurin Gagnon on 2/7/24.
//

#ifndef RENDERER_H
#define RENDERER_H

#include <stdio.h>
#include <string>
#include <queue>
#include <unordered_map>

#include "SpacialMap.h"
#include "SDL2_image/SDL_image.h"
#include "SDL2_ttf/SDL_ttf.h"
#include "Actor.h"
#include "glm/glm.hpp"

struct TextDrawRequest {
    std::string text;
    std::string font_name;
    int x;
    int y;
    int font_size;
    int r;
    int g;
    int b;
    int a;
};

struct ImageDrawRequest
{
    std::string image_name;
    float x;
    float y;
    int rotation_degrees;
    float scale_x;
    float scale_y;
    float pivot_x;
    float pivot_y;
    int r;
    int g;
    int b;
    int a;
    int sorting_order;
    
    static bool compare(ImageDrawRequest a, ImageDrawRequest b) {
        return a.sorting_order < b.sorting_order;
    }
};

struct UIDrawRequest {
    std::string image_name;
    int x;
    int y;
    int r;
    int g;
    int b;
    int a;
    int sorting_order;
    
    static bool compare(UIDrawRequest a, UIDrawRequest b) {
        return a.sorting_order < b.sorting_order;
    }
};

struct PixelDrawRequest {
    int x;
    int y;
    int r;
    int g;
    int b;
    int a;
};

class Renderer {
public:
    static void initialize(const std::string &game_title);
    
    static void renderFrame();
        
    static void showFrame();
    
    static void clearFrame();
    
    static void raveMode();
    
    static bool isVisible(glm::ivec2 dimensions, glm::ivec2 pos); // returns true if actor will be shown on screen
    
    static SDL_RendererFlip GetRendererFlip(bool horizontalFlip, bool verticalFlip);
    
    static void DrawText(std::string str_content, int x, int y, std::string font_name, int font_size, int r, int g, int b, int a) {
        text_requests.push({str_content, font_name, x, y, font_size, r, g, b, a});
    }
    
    static void DrawUI(std::string img_name, float x, float y);
    
    static void DrawUIEx(std::string img_name, float x, float y, float r, float g, float b, float a, float sorting_order);
    
    static void DrawImg(std::string img_name, float x, float y);
    
    static void DrawImgEx(std::string img_name, float x, float y, float rotation_degrees, float scale_x, float scale_y, float pivot_x, float pivot_y, float r, float g, float b, float a, float sorting_order);
    
    static void DrawPixel(float x, float y, float r, float g, float b, float a);
    
    
    // camera functions
    static void setCameraPosition(float x, float y);
    static float getCameraPositionX() {return camera_pos.x;}
    static float getCameraPositionY() {return camera_pos.y;}
    static void setZoom(float zoom) {zoom_factor = zoom;}
    static float getZoom() {return zoom_factor;}
    
private:
    
    static SDL_Texture* loadImage(const std::string &imgName);
    
    static TTF_Font* loadFont(const std::string &text_name, const int size);
    
    static void renderAndClearImgQueue();
    
    static void renderAndClearUIQueue();
    
    static void renderImage(const std::string &imgName);
        
    static void renderText(TTF_Font* font, const TextDrawRequest request);
    
    static void renderActor(const Actor &actor, int x, int y); // actor and xy pixels to render at
    
    static void renderHUD(const std::string &hp_image, TTF_Font* font, int health, int score);
    
    inline static std::unordered_map<std::string, SDL_Texture*> textures;
    
    inline static SDL_Window* window = nullptr;
    inline static SDL_Renderer* renderer = nullptr;
    
    inline static std::unordered_map<std::string, std::unordered_map<int, TTF_Font*>> fonts;
    inline static std::vector<PixelDrawRequest> pixel_requests;
    inline static std::queue<TextDrawRequest> text_requests;
    inline static std::vector<UIDrawRequest> ui_requests;
    inline static std::vector<ImageDrawRequest> img_requests;

    inline static int x_resolution = 640;
    inline static int y_resolution = 360;
    inline static glm::vec2 camera_pos = {0, 0}; // using renderer coordinate system. up = neg, down = pos
    inline static float zoom_factor = 1;    
    inline static int clear_color_r = 255, clear_color_g = 255, clear_color_b = 255;

};


#endif /* RENDERER_H */
