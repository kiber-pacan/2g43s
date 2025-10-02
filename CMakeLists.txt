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

        core/Engine.hpp
        core/Tools.hpp
        core/Logger.hpp

        #core/sep/camera/CamListener.hpp
        core/sep/camera/Camera.h

        core/sep/control/KeyBinding.hpp
        core/sep/control/KeyListener.hpp
        core/sep/control/MouseListener.hpp

        core/sep/device/PhysDevice.hpp
        core/sep/device/LogDevice.hpp

        core/sep/graphics/Graphics.hpp
        core/sep/graphics/Delta.hpp

        core/sep/model/ModelInstance.h
        core/sep/model/ParsedModel.h

        core/sep/util/Debug.hpp
        core/sep/util/Queue.hpp

        core/sep/window/Surface.hpp
        core/sep/window/Swapchain.hpp

        core/sep/entity/Entity.h

        core/sep/util/Color.h
)

add_executable(${EXECUTABLE_NAME})

#Adding sources to project
target_sources(${EXECUTABLE_NAME} PRIVATE ${include})

#Adding SDL3 and glm
if(${WIN32})
    add_subdirectory("libs/SDL")

    add_subdirectory("libs/glm")

    include_directories("libs/stb")
    link_directories("libs/stb")

    add_subdirectory("libs/fastgltf")
else ()
    find_package(SDL3 REQUIRED CONFIG)
    find_package(glm REQUIRED CONFIG)
    add_subdirectory("libs/fastgltf")
    include_directories("/libs/stb")
    #find_package(stb REQUIRED CONFIG)
endif ()

include_directories(${Vulkan_INCLUDE_DIRS})

#Finding Vulkan API and adding it to project
find_package(Vulkan REQUIRED)
if(Vulkan_FOUND)
    include_directories(${Vulkan_INCLUDE_DIRS})
    link_directories(${Vulkan_LIBRARY_DIRS})
endif()

#Linking libraries to project
target_link_libraries(${EXECUTABLE_NAME} PUBLIC
        ${Vulkan_LIBRARIES}
        SDL3::SDL3
        glm::glm
        fastgltf::fastgltf
)
target_compile_definitions(${EXECUTABLE_NAME} PUBLIC SDL_MAIN_USE_CALLBACKS)

# Copy dlls to execution dir
if(${WIN32})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:${PROJECT_NAME}> $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
            COMMAND_EXPAND_LISTS
    )
endif()