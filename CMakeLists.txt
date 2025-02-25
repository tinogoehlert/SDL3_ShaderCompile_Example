cmake_minimum_required(VERSION 3.10)
project(SDL_ShaderCompile_Example)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)

add_subdirectory(external)

add_executable(SDL_ShaderCompile_Example
            src/main.cpp
)

target_link_libraries(SDL_ShaderCompile_Example PUBLIC SDL3::SDL3 SDL3_shadercross::SDL3_shadercross)
     

add_custom_target(copy-runtime-files ALL
     COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/resources ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources)