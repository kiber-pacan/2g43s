#Set cmake version
cmake_minimum_required(VERSION 3.28)

#Setting c++ 23 standard
set(CMAKE_CXX_STANDARD 23)
set(DCMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_EXTENSIONS off)

#Declaring project
project(2g43s)
set(EXECUTABLE_NAME ${PROJECT_NAME})

#Set of source files
set(include
        main.cpp

        core/engine.hpp
        core/tools.hpp
        core/logger.hpp
        core/graphics.hpp

        core/sep/debug.hpp
        core/sep/physDevice.hpp
        core/sep/queue.hpp
        core/sep/logDevice.hpp
        core/sep/surface.hpp
        core/sep/swapchain.hpp
)

add_executable(${EXECUTABLE_NAME})

#Adding sources to project
target_sources(${EXECUTABLE_NAME} PRIVATE ${include})

#Adding SDL3 directory to project
find_package(SDL3 REQUIRED CONFIG)

#include_directories(/core/shaders)

#Finding Vulkan API and adding it to project
find_package(Vulkan REQUIRED)
if(Vulkan_FOUND)
    include_directories(${Vulkan_INCLUDE_DIRS})
    link_directories(${Vulkan_LIBRARY_DIRS})
endif()

#Linking libraries to project
target_link_libraries(${EXECUTABLE_NAME} PUBLIC SDL3::SDL3 ${Vulkan_LIBRARIES})
target_compile_definitions(${EXECUTABLE_NAME} PUBLIC SDL_MAIN_USE_CALLBACKS)
