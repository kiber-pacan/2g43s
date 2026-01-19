# Set cmake version
cmake_minimum_required(VERSION 3.28)

# Setting c++ 26 standard
if(MSVC)
    add_compile_options(/std:c++latest /Zc:__cplusplus /utf-8)
else()
    set(CMAKE_CXX_STANDARD 26)
    set(DCMAKE_CXX_STANDARD 26)
    set(CMAKE_CXX_STANDARD_REQUIRED on)
    set(CMAKE_CXX_EXTENSIONS off)
endif()

# Jolt
include(FetchContent)
FetchContent_Declare(SDL3   GIT_REPOSITORY https://github.com/libsdl-org/SDL.git  GIT_TAG main)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
FetchContent_Declare(JoltPhysics    GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git   GIT_TAG master)
FetchContent_Declare(glm    GIT_REPOSITORY https://github.com/g-truc/glm.git   GIT_TAG master)
FetchContent_Declare(fastgltf   GIT_REPOSITORY https://github.com/spnda/fastgltf.git  GIT_TAG main)
FetchContent_Declare(nlohmann_json  GIT_REPOSITORY https://github.com/nlohmann/json.git  GIT_TAG develop)
FetchContent_Declare(imgui  GIT_REPOSITORY https://github.com/ocornut/imgui.git  GIT_TAG docking)
FetchContent_Declare(stb GIT_REPOSITORY https://github.com/nothings/stb.git GIT_TAG master)

FetchContent_MakeAvailable(SDL3 JoltPhysics glm fastgltf nlohmann_json imgui stb)

FetchContent_GetProperties(stb)
if(NOT stb_POPULATED)
    FetchContent_Populate(stb)
endif()

# Jolt settings
set(INTERACTIVE_SAMPLES OFF CACHE BOOL "" FORCE)
set(SAMPLES OFF CACHE BOOL "" FORCE)
set(UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(GENERATE_DEBUG_SYMBOLS ON CACHE BOOL "" FORCE)
set(USE_SSE4_2 ON CACHE BOOL "" FORCE)


# Declaring project
project(2g43s)

set(EXECUTABLE_NAME ${PROJECT_NAME})

set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(STB_DIR ${stb_SOURCE_DIR})

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

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
        core/sep/util/Tools.hpp

        # Window
        core/sep/window/Swapchain.hpp
        core/sep/window/Swapchain.cpp
        core/sep/entity/PlayerEntity.cpp
        core/sep/entity/PlayerEntity.h

        core/sep/model/Texture.hpp

        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp

        core/sep/util/stb_impl.cpp
)

if(DEFINED ENV{GITHUB_ACTIONS} AND "$ENV{RUNNER_OS}" STREQUAL "Linux")
    message(STATUS "We are running on Ubuntu Runner in GitHub Actions")
    # Твои специфичные настройки для CI
endif()

target_include_directories(${EXECUTABLE_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/libs
        ${CMAKE_CURRENT_SOURCE_DIR}/core
        ${CMAKE_CURRENT_SOURCE_DIR}/core/sep
        ${CMAKE_CURRENT_SOURCE_DIR}/core/sep/graphics
        ${CMAKE_CURRENT_SOURCE_DIR}/core/sep/graphics/pipeline
        ${CMAKE_CURRENT_SOURCE_DIR}/core/sep/graphics/helper
        ${CMAKE_CURRENT_SOURCE_DIR}/core/sep/entity
        ${CMAKE_CURRENT_SOURCE_DIR}/core/sep/device
        ${CMAKE_CURRENT_SOURCE_DIR}/core/sep/controls
        ${CMAKE_CURRENT_SOURCE_DIR}/core/sep/camera
        ${CMAKE_CURRENT_SOURCE_DIR}/core/sep/buffers/culling
        ${CMAKE_CURRENT_SOURCE_DIR}/core/sep/buffers/main
        ${CMAKE_CURRENT_SOURCE_DIR}/core/sep/buffers/main/uniform
        ${CMAKE_CURRENT_SOURCE_DIR}/core/sep/buffers/main/indices
        ${CMAKE_CURRENT_SOURCE_DIR}/core/sep/buffers/matrices
        ${CMAKE_CURRENT_SOURCE_DIR}/core/sep/model/
        ${CMAKE_CURRENT_SOURCE_DIR}/core/sep/model/bus
        ${CMAKE_CURRENT_SOURCE_DIR}/core/sep/model/primitives
        ${CMAKE_CURRENT_SOURCE_DIR}/core/sep/util/
        ${CMAKE_CURRENT_SOURCE_DIR}/core/sep/window/
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
)

target_include_directories(${EXECUTABLE_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/core
        ${CMAKE_CURRENT_SOURCE_DIR}/core/sep
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
)


target_compile_definitions(${EXECUTABLE_NAME} PRIVATE
        PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/"
)

# Optimizations
execute_process(COMMAND cat /etc/machine-id OUTPUT_VARIABLE MACHINE_ID)
string(STRIP "${MACHINE_ID}" MACHINE_ID)
message(STATUS "Hardware ID: ${MACHINE_ID}")
if(NOT MSVC AND ${MACHINE_ID} EQUAL "fe6032524c88414fb6b688ad7002a3b4")
    add_compile_options(
        -O3 -march=native -mtune=native
        -ffast-math -fno-math-errno -fno-trapping-math -fassociative-math -freciprocal-math
        -fno-signed-zeros -fno-rounding-math
        -fopenmp -flto=full -fsplit-lto-unit
        -fstrict-aliasing -fstrict-vtable-pointers -fwhole-program-vtables
        -fno-semantic-interposition -fvisibility=hidden -fvisibility-inlines-hidden
        -funroll-loops -fvectorize -fslp-vectorize
        -fno-stack-protector -fno-stack-check -fno-plt
        -fno-exceptions -fno-rtti -fno-ident
        -fomit-frame-pointer -fmerge-all-constants
        -fplugin=LLVMPolly.so
        -mllvm -polly
        -mllvm -polly-parallel
        -mllvm -polly-vectorizer=stripmine
        -mllvm -polly-invariant-load-hoisting
        -mllvm -polly-run-dce
        -mllvm -polly-check-parallel
        -mllvm --polly-on-isl-error-abort=false
        -mllvm -inline-threshold=2000
        -mllvm -unroll-threshold=1000
        -DNDEBUG)

    add_link_options(
        -fopenmp -flto=full -fuse-ld=mold
        -Wl,-O3 -Wl,--gc-sections -Wl,--icf=all
        -Wl,-Bsymbolic -Wl,-Bsymbolic-functions
        -Wl,--strip-all -Wl,--as-needed)
else()
    add_compile_options(-O3 -fopenmp -fexperimental-library)
    add_link_options(-fopenmp)
endif()

# Adding sources to project
target_sources(${EXECUTABLE_NAME} PRIVATE ${include})

if(NOT DEFINED ENV{VULKAN_SDK})
    message("VULKAN_SDK environment variable not set!")
endif()

set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})

target_include_directories(${PROJECT_NAME} PRIVATE "${VULKAN_SDK_PATH}/include")

if(NOT MSVC AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${EXECUTABLE_NAME} PRIVATE "-fopenmp=libomp")
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE "omp")

    message(STATUS "Clang 21: OpenMP forced via libomp")
else()
    find_package(OpenMP REQUIRED)
    target_link_libraries(${EXECUTABLE_NAME} PUBLIC OpenMP::OpenMP_CXX)
endif()

# Finding Vulkan API and adding it to project
find_package(Vulkan REQUIRED)
if(Vulkan_FOUND)
    include_directories(${Vulkan_INCLUDE_DIRS})
    link_directories(${Vulkan_LIBRARY_DIRS})
endif()



target_link_libraries(${EXECUTABLE_NAME} PUBLIC
        ${Vulkan_LIBRARIES}
        SDL3::SDL3
        glm::glm
        fastgltf::fastgltf
        nlohmann_json::nlohmann_json
        stb
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