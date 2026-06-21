cmake_minimum_required(VERSION 3.20)
project(MateROV LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SDL 2
list(APPEND CMAKE_PREFIX_PATH "$ENV{HOMEBREW_PREFIX}/opt/sdl2")
find_package(SDL2 CONFIG REQUIRED)

# SDL 2 TTF
list(APPEND CMAKE_PREFIX_PATH "$ENV{HOMEBREW_PREFIX}/opt/sdl2_ttf")
find_package(SDL2_ttf CONFIG REQUIRED)

# Boost ASIO
cmake_policy(SET CMP0167 NEW)
find_package(Boost REQUIRED)

add_executable(MateROV main.cpp
        Visual.cpp
        Visual.h
        Controller.cpp
        Controller.h
        CalibManager.cpp
        CalibManager.h
        CalibManager.cpp
        CalibManager.h
        ObsSceneSwitcher.cpp
        ObsSceneSwitcher.h
)

# ImGUI
target_sources(MateROV PRIVATE
        external/imgui/imgui.cpp
        external/imgui/imgui_draw.cpp
        external/imgui/imgui_tables.cpp
        external/imgui/imgui_widgets.cpp
        external/imgui/backends/imgui_impl_sdl2.cpp
        external/imgui/backends/imgui_impl_sdlrenderer2.cpp
)

target_include_directories(MateROV PRIVATE
        external/imgui
        external/imgui/backends
)

# OpenCV
#find_package(OpenCV REQUIRED core imgproc videoio)
#target_include_directories(MateROV PRIVATE ${OpenCV_INCLUDE_DIRS})

target_compile_definitions(MateROV PRIVATE SDL_MAIN_HANDLED)

target_link_libraries(MateROV PRIVATE
        Boost::boost
        SDL2::SDL2
        SDL2_ttf::SDL2_ttf
#        opencv_core
#        opencv_imgproc
#        opencv_videoio
)
