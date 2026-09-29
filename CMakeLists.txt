cmake_minimum_required(VERSION 3.16)
project(mafia LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(shared_ptr INTERFACE)
target_include_directories(shared_ptr INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_sources(shared_ptr INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include/smart_ptr/SharedPtr.hpp)
target_compile_features(shared_ptr INTERFACE cxx_std_17)

function(mafia_set_utf8 target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /utf-8)
    else()
        target_compile_options(${target} PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8)
    endif()
endfunction()

add_executable(mafia
    src/main.cpp
    src/game/Host.cpp
)
target_link_libraries(mafia PRIVATE shared_ptr)
mafia_set_utf8(mafia)

add_executable(test_shared_ptr tests/test_shared_ptr.cpp)
target_link_libraries(test_shared_ptr PRIVATE shared_ptr)
mafia_set_utf8(test_shared_ptr)
