//
//  OGLRenderer.cpp
//  dive_engine
//
//  Created by Mathurin Gagnon on 6/4/24.
//

#include "OGLRenderer.h"
#include "EngineUtils.h"
#include "SDL_opengl.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <filesystem>

using std::filesystem::exists, std::cout, std::endl;

// Vertex and Fragment Shaders
const char* vertexShaderSource = R"glsl(
    #version 330 core
    layout(location = 0) in vec3 position;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main() {
        gl_Position = projection * view * model * vec4(position, 1.0);
    }
)glsl";

const char* fragmentShaderSource = R"glsl(
    #version 330 core
    out vec4 FragColor;

    void main() {
        FragColor = vec4(1.0, 0.5, 0.2, 1.0);
    }
)glsl";

// Cube Vertices
float cubeVertices[] = {
    // Front face
    -0.5f, -0.5f,  0.5f,
     0.5f, -0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,
    // Back face
    -0.5f, -0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
    -0.5f,  0.5f, -0.5f,
};

// Cube Indices (to draw triangles)
unsigned int cubeIndices[] = {
    0, 1, 2, 2, 3, 0, // Front face
    4, 5, 6, 6, 7, 4, // Back face
    0, 1, 5, 5, 4, 0, // Bottom face
    2, 3, 7, 7, 6, 2, // Top face
    0, 3, 7, 7, 4, 0, // Left face
    1, 2, 6, 6, 5, 1  // Right face
};

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

    SDL_Window* cur_window = SDL_CreateWindow("Triangle", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, x_resolution, y_resolution, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

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

    // float pos[] = {
    //     -0.25f, -0.1f, 0.0f, // Vertex 0
    //      0.0f,   0.5f, 0.0f, // Vertex 1
    //      0.25f, -0.1f, 0.0f  // Vertex 2
    // };

    float vertices[] = {
        // Positions
        -0.5f, -0.5f, -0.5f, // Bottom-left-back
        0.5f, -0.5f, -0.5f, // Bottom-right-back
        0.5f,  0.5f, -0.5f, // Top-right-back
        -0.5f,  0.5f, -0.5f, // Top-left-back
        -0.5f, -0.5f,  0.5f, // Bottom-left-front
        0.5f, -0.5f,  0.5f, // Bottom-right-front
        0.5f,  0.5f,  0.5f, // Top-right-front
        -0.5f,  0.5f,  0.5f  // Top-left-front
    };


    unsigned int indices[] = {
        // Back face
        0, 1, 2, 2, 3, 0,
        // Front face
        4, 5, 6, 6, 7, 4,
        // Left face
        0, 3, 7, 7, 4, 0,
        // Right face
        1, 2, 6, 6, 5, 1,
        // Bottom face
        0, 1, 5, 5, 4, 0,
        // Top face
        3, 2, 6, 6, 7, 3
    };


    glEnable(GL_DEPTH_TEST);

    uint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // Vertex Buffer Object
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Element Buffer Object
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Vertex Attribute Pointer
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    const std::string vertexShader = R"glsl(
        #version 330 core

        layout(location = 0) in vec3 position;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        void main() {
            gl_Position = projection * view * model * vec4(position, 1.0);
        }
        )glsl";

    std::string fragmentShader = R"glsl(
        #version 330 core

        out vec4 FragColor;

        void main() {
            FragColor = vec4(1.0, 0.5, 0.2, 1.0); // Orange color
        }
        )glsl";

    uint shader = CreateShader(vertexShader, fragmentShader);
    glUseProgram(shader);

    // Set transformation matrices
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians((float)SDL_GetTicks() / 50.0f), glm::vec3(0.5f, 1.0f, 0.0f));
    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)800 / (float)600, 0.1f, 100.0f);


    GLuint modelLoc = glGetUniformLocation(shader, "model");
    GLuint viewLoc = glGetUniformLocation(shader, "view");
    GLuint projLoc = glGetUniformLocation(shader, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    bool running = true;
    SDL_Event event;

    while (running) {

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        model = glm::rotate(glm::mat4(1.0f), glm::radians((float)SDL_GetTicks() / 50.0f), glm::vec3(0.5f, 1.0f, 0.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO);
    SDL_GL_DeleteContext(cur_context);
    SDL_DestroyWindow(cur_window);
    SDL_Quit();
}

