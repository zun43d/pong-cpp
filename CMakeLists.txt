cmake_minimum_required(VERSION 3.14)
project(pong)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL2 REQUIRED)

add_executable(pong Main.cpp)

target_link_libraries(pong PRIVATE SDL2::SDL2 SDL2_ttf SDL2_mixer)
