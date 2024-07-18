@REM g++ ./src/main.cpp -o ./build/main -I./include  -L./lib  -lglfw3 -lvulkan-1 -lgdi32 -luser32 -static-libstdc++

cmake -G "MinGW Makefiles" -B build
cd .\build
@REM cmake ..
mingw32-make
.\VulkanEngine.exe