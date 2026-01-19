# Set cmake version
cmake_minimum_required(VERSION 3.28)

# Setting c++ 26 standard
set(CMAKE_CXX_STANDARD 26)
set(DCMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_EXTENSIONS off)

# Jolt
include(FetchContent)
FetchContent_Declare(
        JoltPhysics
        GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
        GIT_TAG        master
)

# Jolt settings
set(INTERACTIVE_SAMPLES OFF CACHE BOOL "" FORCE)
set(SAMPLES OFF CACHE BOOL "" FORCE)
set(UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(GENERATE_DEBUG_SYMBOLS ON CACHE BOOL "" FORCE)
set(USE_SSE4_2 ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(JoltPhysics)


# Declaring project
project(2g43s)
set(EXECUTABLE_NAME ${PROJECT_NAME})

add_executable(${EXECUTABLE_NAME}
        main.cpp

        core/Engine.cpp
        core/Engine.hpp

        # Graphics
        core/sep/graphics/Buffers.cpp
        core/sep/graphics/Buffers.hpp
        core/sep/graphics/command/Command.cpp
        core/sep/graphics/command/Command.hpp
        core/sep/graphics/command/Barrier.cpp
        core/sep/graphics/command/Barrier.h
        core/sep/graphics/DeltaManager.hpp
        core/sep/graphics/DeltaManager.cpp

        # Pipeline
        core/sep/graphics/shaders/Shaders.cpp
        core/sep/graphics/shaders/Shaders.hpp
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
        core/sep/device/PhysicalDevice.hpp
        core/sep/device/LogicalDevice.hpp


        # Controls
        core/sep/controls/KeyBinding.hpp
        core/sep/controls/KeyListener.hpp
        core/sep/controls/MouseListener.hpp


        # Camera
        core/sep/camera/Camera.hpp


        # Buffer
        core/sep/buffers/culling/ModelCullingBufferObject.hpp
        core/sep/buffers/culling/VisibleIndicesBufferObject.hpp

        core/sep/buffers/main/AtomicCounterObject.hpp
        core/sep/buffers/main/DrawCommandsBufferObject.hpp
        core/sep/buffers/main/uniform/UniformBufferObject.hpp
        core/sep/buffers/main/uniform/UniformPostprocessingBufferObject.hpp
        core/sep/buffers/main/uniform/UniformCullingBufferObject.hpp
        core/sep/buffers/main/indices/TextureIndexOffsetObject.hpp
        core/sep/buffers/main/indices/TextureIndexObject.h

        core/sep/buffers/matrices/ModelBufferObject.hpp
        core/sep/buffers/matrices/ModelDataBufferObject.hpp

        # Model
        core/sep/model/ModelInstance.hpp
        core/sep/model/ParsedModel.hpp
        core/sep/model/ParsedModel.cpp
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

        # Window
        core/sep/window/Swapchain.hpp
        core/sep/window/Swapchain.cpp
        core/sep/entity/PlayerEntity.cpp
        core/sep/entity/PlayerEntity.h

        core/sep/model/Texture.hpp
        core/sep/graphics/shaders/constants/CullingPushConstants.hpp
        core/sep/graphics/shaders/constants/VertexPushConstants.hpp
        core/sep/graphics/shaders/constants/MatrixPushConstants.hpp
        core/sep/graphics/shaders/constants/PostprocessPushConstants.hpp
)


target_include_directories(${EXECUTABLE_NAME} PRIVATE
        libs/
        core/
        core/sep

        core/sep/graphics
        core/sep/graphics/pipeline
        core/sep/graphics/helper

        core/sep/entity

        core/sep/device

        core/sep/controls

        core/sep/camera

        core/sep/buffers/culling
        core/sep/buffers/main
        core/sep/buffers/main/uniform
        core/sep/buffers/main/indices
        core/sep/buffers/matrices

        core/sep/model/
        core/sep/model/bus
        core/sep/model/primitives

        core/sep/util/

        core/sep/window/
)

target_compile_definitions(${EXECUTABLE_NAME} PRIVATE
        PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/"
)

# Optimizations
if(NOT MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE " \
        -O3 -march=native -mtune=native \
        -ffast-math -fno-math-errno -fno-trapping-math -fassociative-math -freciprocal-math \
        -fno-signed-zeros -fno-rounding-math \
        -fopenmp -flto=full -fsplit-lto-unit \
        -fstrict-aliasing -fstrict-vtable-pointers -fwhole-program-vtables \
        -fno-semantic-interposition -fvisibility=hidden -fvisibility-inlines-hidden \
        -funroll-loops -fvectorize -fslp-vectorize \
        -fno-stack-protector -fno-stack-check -fno-plt \
        -fno-exceptions -fno-rtti -fno-ident \
        -fomit-frame-pointer -fmerge-all-constants \
        -fplugin=LLVMPolly.so \
        -mllvm -polly \
        -mllvm -polly-parallel \
        -mllvm -polly-vectorizer=stripmine \
        -mllvm -polly-invariant-load-hoisting \
        -mllvm -polly-run-dce \
        -mllvm -polly-check-parallel \
        -mllvm --polly-on-isl-error-abort=false \
        -mllvm -inline-threshold=2000 \
        -mllvm -unroll-threshold=1000 \
        -DNDEBUG")

    set(CMAKE_EXE_LINKER_FLAGS_RELEASE " \
        -fopenmp -flto=full -fuse-ld=mold \
        -Wl,-O3 -Wl,--gc-sections -Wl,--icf=all \
        -Wl,-Bsymbolic -Wl,-Bsymbolic-functions \
        -Wl,--strip-all -Wl,--as-needed")
endif()

# Adding sources to project
target_sources(${EXECUTABLE_NAME} PRIVATE ${include})

if(NOT DEFINED ENV{VULKAN_SDK})
    message("VULKAN_SDK environment variable not set!")
endif()

set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})

target_include_directories(${PROJECT_NAME} PRIVATE "${VULKAN_SDK_PATH}/include")


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
    find_package(OpenMP REQUIRED)
    find_library(SPIRV_TOOLS_LIB NAMES SPIRV-Tools)
    find_library(SPIRV_TOOLS_OPT_LIB NAMES SPIRV-Tools-opt)
    find_library(GLSLANG_LIB NAMES glslang)

    find_package(nlohmann_json REQUIRED CONFIG)

    add_subdirectory("libs/fastgltf")
    include_directories("libs/stb")
    if(OpenMP_CXX_FOUND)
        target_link_libraries(${EXECUTABLE_NAME} PRIVATE OpenMP::OpenMP_CXX)
    endif()
endif ()

include_directories(${Vulkan_INCLUDE_DIRS})

# Finding Vulkan API and adding it to project
find_package(Vulkan REQUIRED)
if(Vulkan_FOUND)
    include_directories(${Vulkan_INCLUDE_DIRS})
    link_directories(${Vulkan_LIBRARY_DIRS})
endif()

target_link_libraries(${EXECUTABLE_NAME} PUBLIC
        ${Vulkan_LIBRARIES}
        Jolt
        SDL3::SDL3
        glm::glm
        fastgltf::fastgltf
        imgui::imgui
        nlohmann_json::nlohmann_json
)

target_link_libraries(${EXECUTABLE_NAME} PRIVATE
        shaderc_combined
        ${GLSLANG_LIB}
        ${SPIRV_TOOLS_OPT_LIB}
        ${SPIRV_TOOLS_LIB}
        OSDependent
        MachineIndependent
        GenericCodeGen
        pthread
)
target_compile_definitions(${EXECUTABLE_NAME} PUBLIC SDL_MAIN_USE_CALLBACKS)

# Copy dlls to execution dir
if(${WIN32})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:${PROJECT_NAME}> $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
            COMMAND_EXPAND_LISTS
    )
endif()