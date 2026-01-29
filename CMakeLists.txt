# Set cmake version
cmake_minimum_required(VERSION 3.28)

# Setting c++ 26 standard
set(CMAKE_CXX_STANDARD 26)
set(DCMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_EXTENSIONS off)

# Declaring project
project(2g43s)
cmake_policy(SET CMP0167 OLD)

# Jolt settings
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Distribution")
set(DOUBLE_PRECISION OFF)
set(GENERATE_DEBUG_SYMBOLS ON)
set(OVERRIDE_CXX_FLAGS ON)
set(CROSS_PLATFORM_DETERMINISTIC OFF)
set(FLOATING_POINT_EXCEPTIONS_ENABLED OFF)
set(CPP_EXCEPTIONS_ENABLED OFF)
set(CPP_RTTI_ENABLED OFF)
set(OBJECT_LAYER_BITS 16)
set(SUBPROJECT ON CACHE BOOL "" FORCE)
set(TARGET_SAMPLES OFF CACHE BOOL "" FORCE)        # Не SAMPLES!
set(TARGET_UNIT_TESTS OFF CACHE BOOL "" FORCE)     # Не UNIT_TESTS!
set(TARGET_HELLO_WORLD OFF CACHE BOOL "" FORCE)
set(GENERATE_SHADERS OFF CACHE BOOL "" FORCE)

set(USE_SSE4_2 ON CACHE BOOL "" FORCE)
set(INTERPROCEDURAL_OPTIMIZATION ON CACHE BOOL "" FORCE)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

set(FASTGLTF_ENABLE_MESHOPT ON CACHE BOOL "" FORCE)

include(FetchContent)

FetchContent_Declare(meshoptimizer  GIT_REPOSITORY https://github.com/zeux/meshoptimizer.git    GIT_TAG master)
FetchContent_MakeAvailable(meshoptimizer)

FetchContent_Declare(jolt_physics   GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git    GIT_TAG "v5.5.0"  SOURCE_SUBDIR "Build")
FetchContent_Declare(SDL3   GIT_REPOSITORY https://github.com/libsdl-org/SDL.git    GIT_TAG main)
FetchContent_Declare(glm    GIT_REPOSITORY https://github.com/g-truc/glm.git   GIT_TAG master)

FetchContent_Declare(fastgltf   GIT_REPOSITORY https://github.com/spnda/fastgltf.git  GIT_TAG main)

FetchContent_Declare(nlohmann_json  GIT_REPOSITORY https://github.com/nlohmann/json.git  GIT_TAG develop)
FetchContent_Declare(imgui  GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG  docking)
FetchContent_Declare(basis  GIT_REPOSITORY https://github.com/BinomialLLC/basis_universal.git  GIT_TAG master)
#FetchContent_Declare(boost  GIT_REPOSITORY https://github.com/boostorg/boost.git  GIT_TAG master)

FetchContent_MakeAvailable(jolt_physics SDL3 glm fastgltf nlohmann_json imgui basis)

# IMGUI
add_library(imgui_lib STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)
target_include_directories(imgui_lib PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui_lib PRIVATE SDL3::SDL3 Vulkan::Vulkan)

# BASIS
add_library(basis_transcoder STATIC
        ${basis_SOURCE_DIR}/transcoder/basisu_transcoder.cpp
        ${basis_SOURCE_DIR}/zstd/zstd.h
        ${basis_SOURCE_DIR}/zstd/zstd.c
)

target_include_directories(basis_transcoder SYSTEM PUBLIC
        ${basis_SOURCE_DIR}/transcoder
        ${basis_SOURCE_DIR}/zstd
)

target_compile_options(basis_transcoder PRIVATE
        -Wno-enum-enum-conversion -Wno-deprecated-declarations
)

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
        core/sep/images/Images.cpp
        core/sep/images/Images.hpp


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

        core/sep/buffers/main/AtomicCounterBufferObject.hpp
        core/sep/buffers/main/DrawCommandsBufferObject.hpp
        core/sep/buffers/main/uniform/UniformBufferObject.hpp
        core/sep/buffers/main/uniform/UniformPostprocessingBufferObject.hpp
        core/sep/buffers/main/uniform/UniformCullingBufferObject.hpp
        core/sep/buffers/main/indices/TextureIndexOffsetBufferObject.hpp
        core/sep/buffers/main/indices/TextureIndexBufferObject.h

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

        core/sep/images/Texture.hpp
        core/sep/model/managing/PhysicsBus.cpp
        core/sep/model/managing/PhysicsBus.h
        core/sep/model/managing/PhysicsBus.h
        core/sep/graphics/helper/Sync.hpp
        core/sep/model/managing/ModelEntityManager.hpp
        core/sep/model/managing/ModelEntityManager.cpp
        core/sep/model/managing/ModelGroup.h
)

target_include_directories(${EXECUTABLE_NAME} PRIVATE
        libs
        core
        core/sep
        core/sep/graphics
        core/sep/graphics/pipeline
        core/sep/graphics/helper
        core/sep/graphics/command
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
        core/sep/model/managing
        core/sep/model/primitives
        core/sep/util/
        core/sep/window/
        core/sep/graphics/shaders/constants/
        core/sep/images/
        core/sep/graphics/shaders/
)

find_package(Boost REQUIRED)
target_include_directories(${EXECUTABLE_NAME} SYSTEM PRIVATE
        ${jolt_physics_SOURCE_DIR}
        ${Boost_INCLUDE_DIRS}
)


# For using in c++ code
target_compile_definitions(${EXECUTABLE_NAME} PRIVATE
        PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/"
)
if(UNIX)
add_compile_options(${EXECUTABLE_NAME} PRIVATE
        -Wno-enum-enum-conversion -Wno-deprecated-declarations
        -march=native -mtune=native
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

add_link_options(${EXECUTABLE_NAME} PRIVATE
        -fopenmp -flto=full -fuse-ld=mold
        -Wl,-O3 -Wl,--gc-sections -Wl,--icf=all
        -Wl,-Bsymbolic -Wl,-Bsymbolic-functions
        -Wl,--strip-all -Wl,--as-needed)

else()

endif()

# Adding sources to project
target_sources(${EXECUTABLE_NAME} PRIVATE ${include})

# Finding Vulkan API and adding it to project
find_package(Vulkan REQUIRED)
find_package(OpenMP REQUIRED)

target_link_libraries(${EXECUTABLE_NAME} PUBLIC
        Jolt
        meshoptimizer
        Vulkan::Vulkan
        shaderc_combined
        glslang
        SPIRV-Tools
        SPIRV-Tools-opt
        SDL3::SDL3
        glm::glm
        fastgltf
        nlohmann_json
        imgui_lib
        basis_transcoder
        OpenMP::OpenMP_CXX
        Boost::boost
)

target_compile_definitions(${EXECUTABLE_NAME} PUBLIC SDL_MAIN_USE_CALLBACKS)

# Copy dlls to execution dir
if(${WIN32})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:${PROJECT_NAME}> $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
            COMMAND_EXPAND_LISTS
    )
endif()