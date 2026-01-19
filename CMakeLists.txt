cmake_minimum_required(VERSION 3.28)
project(2g43s)

set(EXECUTABLE_NAME ${PROJECT_NAME})

# --- Стандарт C++ 26 ---
if(MSVC)
    add_compile_options(/std:c++latest /Zc:__cplusplus /utf-8 /permissive-)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    set(CMAKE_CXX_STANDARD 26)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

# --- Зависимости (FetchContent) ---
include(FetchContent)

FetchContent_Declare(SDL3 GIT_REPOSITORY https://github.com/libsdl-org/SDL.git GIT_TAG main)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
FetchContent_Declare(JoltPhysics GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git GIT_TAG master)
FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG master)
FetchContent_Declare(fastgltf GIT_REPOSITORY https://github.com/spnda/fastgltf.git GIT_TAG main)
FetchContent_Declare(nlohmann_json GIT_REPOSITORY https://github.com/nlohmann/json.git GIT_TAG develop)
FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG docking)

# MakeAvailable для всех, кроме ImGui
FetchContent_MakeAvailable(SDL3 JoltPhysics glm fastgltf nlohmann_json)

FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()
set(IMGUI_DIR ${imgui_SOURCE_DIR})

# Настройки Jolt
set(INTERACTIVE_SAMPLES OFF CACHE BOOL "" FORCE)
set(SAMPLES OFF CACHE BOOL "" FORCE)
set(UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(USE_SSE4_2 ON CACHE BOOL "" FORCE)

# --- Исполняемый файл ---
add_executable(${EXECUTABLE_NAME}
        main.cpp
        core/Engine.cpp
        core/Engine.hpp
        core/sep/graphics/Buffers.cpp
        core/sep/graphics/Buffers.hpp
        core/sep/graphics/command/Command.cpp
        core/sep/graphics/command/Command.hpp
        core/sep/graphics/command/Barrier.cpp
        core/sep/graphics/command/Barrier.h
        core/sep/graphics/DeltaManager.hpp
        core/sep/graphics/DeltaManager.cpp
        core/sep/graphics/shaders/Shaders.cpp
        core/sep/graphics/shaders/Shaders.hpp
        core/sep/graphics/pipeline/PipelineCreation.cpp
        core/sep/graphics/pipeline/PipelineCreation.hpp
        core/sep/graphics/pipeline/Descriptor.cpp
        core/sep/graphics/pipeline/Descriptor.hpp
        core/sep/graphics/helper/Helper.cpp
        core/sep/graphics/helper/Helper.hpp
        core/sep/graphics/helper/Images.cpp
        core/sep/graphics/helper/Images.hpp
        core/sep/entity/CameraEntity.hpp
        core/sep/entity/Entity.hpp
        core/sep/device/PhysicalDevice.hpp
        core/sep/device/LogicalDevice.hpp
        core/sep/controls/KeyBinding.hpp
        core/sep/controls/KeyListener.hpp
        core/sep/controls/MouseListener.hpp
        core/sep/camera/Camera.hpp
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
        core/sep/model/ModelInstance.hpp
        core/sep/model/ParsedModel.hpp
        core/sep/model/ParsedModel.cpp
        core/sep/model/bus/ModelBus.cpp
        core/sep/model/bus/ModelBus.hpp
        core/sep/model/primitives/Sphere.hpp
        core/sep/model/primitives/Vertex.hpp
        core/sep/util/Color.hpp
        core/sep/util/Debug.hpp
        core/sep/util/Logger.hpp
        core/sep/util/Queue.hpp
        core/sep/util/Random.hpp
        core/sep/util/stb_impl.cpp
        core/sep/util/Tools.hpp
        core/sep/window/Swapchain.hpp
        core/sep/window/Swapchain.cpp
        core/sep/entity/PlayerEntity.cpp
        core/sep/entity/PlayerEntity.h
        core/sep/model/Texture.hpp
        # ImGui
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

# --- ИНКЛЮДЫ (Исправляем STB и пути) ---
target_include_directories(${EXECUTABLE_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/core
        ${CMAKE_CURRENT_SOURCE_DIR}/core/sep
        ${CMAKE_CURRENT_SOURCE_DIR}/libs       # Для #include "stb/stb_image.h"
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/stb   # Для #include "stb_image.h"
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
)

target_compile_definitions(${EXECUTABLE_NAME} PRIVATE
        PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/"
        SDL_MAIN_USE_CALLBACKS
)

# --- ОПТИМИЗАЦИИ (Fix Polly conflict) ---
if(NOT MSVC)
    # УДАЛЕНО -fplugin=LLVMPolly.so (он встроен в Clang 21 и вызывает ошибку при дублировании)
    # УДАЛЕНО -ffast-math (он ломает json и fastgltf, так как запрещает inf/nan)
    set(CLANG_OPT_FLAGS "-O3 -march=native -fopenmp=libomp -flto=full -fstrict-aliasing -mllvm -polly -mllvm -polly-vectorizer=stripmine")

    set(CMAKE_CXX_FLAGS_RELEASE "${CLANG_OPT_FLAGS} -DNDEBUG")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-fopenmp=libomp -flto=full -fuse-ld=lld -Wl,-O3")
endif()

# --- Внешние библиотеки ---
find_package(Vulkan REQUIRED COMPONENTS shaderc_combined)
target_include_directories(${EXECUTABLE_NAME} PRIVATE $ENV{VULKAN_SDK}/include)

if(NOT MSVC AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE omp)
else()
    find_package(OpenMP REQUIRED)
    target_link_libraries(${EXECUTABLE_NAME} PUBLIC OpenMP::OpenMP_CXX)
endif()

target_link_libraries(${EXECUTABLE_NAME} PUBLIC
        Jolt SDL3::SDL3-shared glm::glm fastgltf::fastgltf nlohmann_json::nlohmann_json Vulkan::Vulkan Vulkan::shaderc_combined pthread
)

# Копирование DLL для Windows
if(WIN32)
    add_custom_command(TARGET ${EXECUTABLE_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:${EXECUTABLE_NAME}> $<TARGET_RUNTIME_DLLS:${EXECUTABLE_NAME}>
            COMMAND_EXPAND_LISTS
    )
endif()