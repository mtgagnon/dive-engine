//
//  OGLRenderer.h
//  dive_engine
//
//  Created by Mathurin Gagnon on 6/4/24.
//

#ifndef OGLRENDERER_H
#define OGLRENDERER_H

#include <stdio.h>
#include <string>

#include "glm/glm.hpp"
#include "GL/glew.h"
#include "SDL2/SDL.h"
#include "Actor.h"

class OGLRenderer {
public:

    static void initialize(const std::string &game_title);
    
    static void destroyRenderer() {
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
        
    static void renderFrame();
        
    static void showFrame();
    
    static void clearFrame();
                
    static int GetFrameNumber() {return frame_number;};
    
    static void drawTriangle();

    // camera functions
//    static void setCameraPosition(float x, float y);
//    static float getCameraPositionX() {return camera_pos.x;}
//    static float getCameraPositionY() {return camera_pos.y;}
//    static void setZoom(float zoom) {zoom_factor = zoom;}
//    static float getZoom() {return zoom_factor;}
    
private:
    
    static void renderAndClearImgQueue();
    
    static void renderAndClearUIQueue();
    
    static void renderImage(const std::string &imgName);
    
    static void renderActor(const Actor &actor, int x, int y); // actor and xy pixels to render at
        
//    inline static std::unordered_map<std::string, SDL_Texture*> textures;
    
    inline static SDL_Window* window = nullptr;
    inline static SDL_GLContext context = nullptr;
    
//    inline static std::unordered_map<std::string, std::unordered_map<int, TTF_Font*>> fonts;
//    inline static std::vector<PixelDrawRequest> pixel_requests;
//    inline static std::queue<TextDrawRequest> text_requests;
//    inline static std::vector<UIDrawRequest> ui_requests;
//    inline static std::vector<ImageDrawRequest> img_requests;

    inline static int x_resolution = 640*2;
    inline static int y_resolution = 360*2;
//    inline static glm::vec2 camera_pos = {0, 0};
//    inline static float zoom_factor = 1;
//    inline static int clear_color_r = 255, clear_color_g = 255, clear_color_b = 255;

    inline static int frame_number = 0;
    inline static Uint32 current_frame_start_timestamp = 0;
};


#endif /* OGLRENDERER_H */
