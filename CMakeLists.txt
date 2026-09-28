cmake_minimum_required(VERSION 3.16)
project(mafia LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(mafia src/main.cpp)

if(MSVC)
    target_compile_options(mafia PRIVATE /utf-8)
else()
    target_compile_options(mafia PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()
