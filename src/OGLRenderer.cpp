//
//  OGLRenderer.cpp
//  dive_engine
//
//  Created by Mathurin Gagnon on 6/4/24.
//

#include "OGLRenderer.h"
#include "EngineUtils.h"
#include "SDL_opengl.h"

#include <filesystem>

using std::filesystem::exists, std::cout, std::endl;

void OGLRenderer::initialize(const std::string &game_title) {
    // defaul display settings
    x_resolution = 640;
    y_resolution = 360;
    
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
    }
    
    
    //INITIALIZE WINDOW
    if(SDL_Init(SDL_INIT_VIDEO)) {
        std::cout << "SDL could not initialize! SDL_ERROR: " << SDL_GetError() << std::endl;
        exit(1);
    }
    
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    
    window = SDL_CreateWindow(game_title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, x_resolution, y_resolution, SDL_WINDOW_SHOWN);
    
    if(!window) {
        std::cerr << "Window could not be created! SDL Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        exit(1);
    }
    
    // CREATE CONTEXT
    context = SDL_GL_CreateContext(window);
    
    if (!context) {
        std::cerr << "Failed to create OpenGL context: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        exit(1);
    }
    
    if(glewInit() != GLEW_OK) {
        exit(1);
    }
    
    std::cout << glGetString(GL_VERSION) << std::endl;
}

static uint CompileShader(uint type, const std::string &source) {
    uint id = glCreateShader(type);
    const char* src = source.c_str();

    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);
    
    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if(result == GL_FALSE) {
        int len;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &result);
        char* message = (char*)alloca(len*sizeof(char));
        glGetShaderInfoLog(id, len, &len, message);
        
        cout << "Failed to compile " << ((type == GL_VERTEX_SHADER) ? "vertex" : "fragment") << " shader!\n";
        cout << message << endl;
        
        glDeleteShader(id);
        return 0;
    }
    
    return id;
}

static uint CreateShader(const std::string &vertexShader, const std::string &fragmentShader) {
    uint program = glCreateProgram();
    uint vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
    uint fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);
    
    glAttachShader(program, vs);
    glAttachShader(program, fs);

    glLinkProgram(program);
    glValidateProgram(program);
    
    glDeleteShader(vs);
    glDeleteShader(fs);
    
    return program;
}

void OGLRenderer::drawTriangle() {
    //INITIALIZE WINDOW
    if(SDL_Init(SDL_INIT_VIDEO)) {
        std::cout << "SDL could not initialize! SDL_ERROR: " << SDL_GetError() << std::endl;
        exit(1);
    }
    
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    
    SDL_Window* cur_window = SDL_CreateWindow("Triangle", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, x_resolution, y_resolution, SDL_WINDOW_SHOWN);
    
    if(!cur_window) {
        std::cerr << "Window could not be created! SDL Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        exit(1);
    }
    
    // CREATE CONTEXT
    SDL_GLContext cur_context = SDL_GL_CreateContext(cur_window);
    
    if (!cur_context) {
        std::cerr << "Failed to create OpenGL context: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        exit(1);
    }
    
    glewExperimental = GL_TRUE;
    if(glewInit() != GLEW_OK) {
        exit(1);
    }
    
    std::cout << glGetString(GL_VERSION) << std::endl;
    
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    bool running = true;
    SDL_Event event;
    
    float pos[6] = {
        -0.25f, -0.1f,
         0.0f,  0.5f,
         0.25f, -0.1f
    };
    
    uint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    
    uint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, 6*sizeof(float), pos, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float)*2, 0);
    
    const std::string vertexShader = 
        R"glsl(
        #version 330 core

        layout(location = 0) in vec4 position;

        void main(){
           gl_Position = position;
        }
        )glsl";
    
    std::string fragmentShader =
        R"glsl(
        #version 330 core

        layout(location = 0) out vec4 color;

        void main(){
           color = vec4(1.0, 0.0, 0.0, 1.0);
        }
        )glsl";

    uint shader = CreateShader(vertexShader, fragmentShader);
    glUseProgram(shader);
    
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }
        
        glClear(GL_COLOR_BUFFER_BIT);
        
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        
        SDL_GL_SwapWindow(cur_window);
    }
    
    SDL_GL_DeleteContext(cur_context);
    SDL_DestroyWindow(cur_window);
    SDL_Quit();
}

