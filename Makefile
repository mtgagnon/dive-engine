# this one is to try and appease the autograder which doesn't like engine.o which works on my machine for some

main:
	clang++ -std=c++17 -I./ -I./external/ -I./external/box2d/ -I./external/box2d/src/ -I./external/box2d/src/collision/ -I./external/box2d/src/common/ -I./external/box2d/src/dynamics/ -I./external/box2d/src/rope/ -I./external/SDL2 -I./external/SDL_image -I./external/SDL_mixer -I./external/SDL_ttf -L./external/lib -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -llua5.4 -I./external/glm-0.9.9.8/ -I./external/rapidjson-1.1.0/include/ -I./external/LuaBridge/ -I./external/LuaBridge/detail -I./Lua/ -I./HeaderFiles/ ./external/box2d/src/collision/*.cpp ./external/box2d/src/common/*.cpp ./external/box2d/src/dynamics/*.cpp ./external/box2d/src/rope/*.cpp ./sourceFiles/*.cpp -O3 -o game_engine_linux
clean:
	rm -rf *o a.out game_engine_linux