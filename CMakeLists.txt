#Set cmake version
cmake_minimum_required(VERSION 3.28)

#Setting c++ 26 standard
set(CMAKE_CXX_STANDARD 26)
set(DCMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_EXTENSIONS off)

#Declaring project
project(2g43s)
set(EXECUTABLE_NAME ${PROJECT_NAME})

add_executable(${EXECUTABLE_NAME}
        main.cpp

        core/Engine.cpp
        core/Engine.hpp

        # Graphics
        core/sep/graphics/Buffers.cpp
        core/sep/graphics/Buffers.hpp
        core/sep/graphics/Command.cpp
        core/sep/graphics/Command.hpp
        core/sep/graphics/Delta.hpp
        core/sep/graphics/Graphics.hpp


        # Pipeline
        core/sep/graphics/pipeline/Shaders.cpp
        core/sep/graphics/pipeline/Shaders.hpp
        core/sep/graphics/pipeline/PipelineCreation.cpp
        core/sep/graphics/pipeline/PipelineCreation.hpp
        core/sep/graphics/pipeline/Descriptor.cpp
        core/sep/graphics/pipeline/Descriptor.hpp

        # Helper
        core/sep/graphics/helper/Helper.cpp
        core/sep/graphics/helper/Helper.hpp
        core/sep/graphics/helper/Images.cpp
        core/sep/graphics/helper/Images.hpp


        # Entity
        core/sep/entity/CameraEntity.hpp
        core/sep/entity/Entity.hpp


        # Device
        core/sep/device/PhysDevice.hpp
        core/sep/device/LogDevice.hpp


        # Control
        core/sep/control/KeyBinding.hpp
        core/sep/control/KeyListener.hpp
        core/sep/control/MouseListener.hpp


        # Camera
        core/sep/camera/Camera.hpp


        # Buffer
        core/sep/buffers/culling/ModelCullingBufferObject.hpp
        core/sep/buffers/culling/VisibleIndicesBufferObject.hpp

        core/sep/buffers/generic/AtomicCounterObject.hpp
        core/sep/buffers/generic/DrawCommandsBufferObject.hpp
        core/sep/buffers/generic/UniformBufferObject.hpp
        core/sep/buffers/generic/UniformCullingBufferObject.hpp

        core/sep/buffers/matrices/ModelBufferObject.hpp
        core/sep/buffers/matrices/ModelDataBufferObject.hpp

        # Model
        core/sep/model/ModelInstance.hpp
        core/sep/model/ParsedModel.hpp
        core/sep/model/bus/ModelBus.cpp
        core/sep/model/bus/ModelBus.hpp
        core/sep/model/primitives/Sphere.hpp
        core/sep/model/primitives/Vertex.hpp


        # Util
        core/sep/util/Color.hpp
        core/sep/util/Debug.hpp
        core/sep/util/Logger.hpp
        core/sep/util/Queue.hpp
        core/sep/util/Random.hpp
        core/sep/util/stb_impl.cpp
        core/sep/util/Tools.hpp
)

target_include_directories(${EXECUTABLE_NAME} PRIVATE
        core/sep

        core/sep/graphics
        core/sep/graphics/pipeline
        core/sep/graphics/helper

        core/sep/entity

        core/sep/device

        core/sep/control

        core/sep/camera

        core/sep/buffers/culling
        core/sep/buffers/generic
        core/sep/buffers/matrices

        core/sep/model/
        core/sep/model/bus
        core/sep/model/primitives

        core/sep/util/
)

# Optimizations
set(CMAKE_CXX_FLAGS_RELEASE " \
    -O3 \
    -march=native \
    -mtune=native \
    -ffast-math \
    -fno-finite-math-only \
    -fopenmp \
    -flto=full \
    -fstrict-aliasing \
    -fstrict-vtable-pointers \
    -fwhole-program-vtables \
    -funroll-loops \
    -fvectorize \
    -fslp-vectorize \
    -fno-stack-protector \
    -fno-stack-check \
    -fno-plt \
    -fno-exceptions \
    -fno-rtti \
    -fomit-frame-pointer \
    -fplugin=LLVMPolly.so \
    -mllvm -polly \
    -mllvm -polly-parallel \
    -mllvm -polly-vectorizer=stripmine \
    -mllvm -polly-invariant-load-hoisting \
    -mllvm -polly-run-dce \
    -mllvm -inline-threshold=1000 \
    -DNDEBUG")

# Оптимизация линковки (Максимальная агрессия)
set(CMAKE_EXE_LINKER_FLAGS_RELEASE " \
    -fopenmp \
    -flto=full \
    -fuse-ld=mold \
    -Wl,-O3 \
    -Wl,--gc-sections \
    -Wl,--icf=all \
    -Wl,--strip-all")

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
    find_package(imgui REQUIRED CONFIG)
    add_subdirectory("libs/fastgltf")
    include_directories("/libs/stb")
    find_package(OpenMP REQUIRED)
    if(OpenMP_CXX_FOUND)
        target_link_libraries(${EXECUTABLE_NAME} PRIVATE OpenMP::OpenMP_CXX)
    endif()
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
        imgui::imgui
)
target_compile_definitions(${EXECUTABLE_NAME} PUBLIC SDL_MAIN_USE_CALLBACKS)

# Copy dlls to execution dir
if(${WIN32})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:${PROJECT_NAME}> $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
            COMMAND_EXPAND_LISTS
    )
endif()