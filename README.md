# Dive Engine

This project uses CMake to build the project on your machine so making it should be fairly simple!
Setting up the project: 
## MacOS
   1. Create and move to a build folder
    
   2. Once in the build folder run the cmake command to build using CMakeLists.txt: `cmake -G "Xcode" ..`
    
   3. Open up your project! Make sure your working directory is where your resources folder
      is or it won't find your resources folder!

## Linux
   1. Create and move to a build folder, make sure to have these packages: `sudo apt-get install mesa-common-dev freeglut3-dev`
    
   2. Once in the build folder run the cmake command to build using CMakeLists.txt: `cmake ..`
    
   3. Run the executable with: `./dive_engine`

Have fun!
