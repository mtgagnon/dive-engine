# Dive Engine

This project uses CMake to build the project on your machine so making it should be fairly simple!
Setting up the project:
## MacOS
   1. Create a build folder, run `eval $BUILD_MAC`

   2. Open up your project! Make sure your working directory is where your resources folder
      is or it won't find your resources folder!

## Linux
   1. Create and move to a build folder, make sure to have these packages: `sudo apt-get install mesa-common-dev freeglut3-dev`, in the root directory run:
      - ### Debug: `eval $BUILD_LINUX_D`
      - ### Release: `eval $BUILD_LINUX_R`

   2. Optionally you can do this:
      - Once in the build folder run the cmake command to build using CMakeLists.txt:
      `cmake -G "Unix Makefiles" -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -S .. -B build-debug -DCMAKE_BUILD_TYPE=Debug`
      `cmake -G "Unix Makefiles" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -S .. -B build-release -DCMAKE_BUILD_TYPE=Release`

   3. Run the executable with: `./dive_engine`

Have fun!
