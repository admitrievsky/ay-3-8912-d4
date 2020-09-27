cmake_minimum_required(VERSION 3.17)
project(win-implementation)

set(CMAKE_CXX_STANDARD 14)

add_executable(win-implementation main.cpp music_data.h music_data.cpp ay_channel.cpp ay_channel.h defines.h music_state.cpp music_state.h)