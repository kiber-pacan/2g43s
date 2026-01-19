cmake_minimum_required(VERSION 3.28)
project(2g43s)

# --- C++26 ---
if(MSVC)
    add_compile_options(/std:c++latest /Zc:__cplusplus /utf-8)
else()
    set(CMAKE_CXX_STANDARD 26)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

include(FetchContent)

# 1. SDL3
FetchContent_Declare(SDL3 GIT_REPOSITORY https://github.com/libsdl-org/SDL.git GIT_TAG main)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)

# 2. Jolt
FetchContent_Declare(JoltPhysics GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git GIT_TAG master)
set(INTERACTIVE_SAMPLES OFF CACHE BOOL "" FORCE)
set(SAMPLES OFF CACHE BOOL "" FORCE)
set(UNIT_TESTS OFF CACHE BOOL "" FORCE)

# 3. GLM
FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG master)

# 4. fastgltf
FetchContent_Declare(fastgltf GIT_REPOSITORY https://github.com/spnda/fastgltf.git GIT_TAG v0.8.0)

# 5. nlohmann_json
FetchContent_Declare(nlohmann_json GIT_REPOSITORY https://github.com/nlohmann/json.git GIT_TAG v3.11.3)

# 6. ImGui (НЕ добавляем в MakeAvailable!)
FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG docking)

# Делаем доступными только те либы, у которых есть CMakeLists.txt
FetchContent_MakeAvailable(SDL3 JoltPhysics glm fastgltf nlohmann_json)

# Для ImGui просто получаем путь к исходникам
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()

# --- Поиск Vulkan с компонентами ---
# Это важно! Нам нужны shaderc и glslang для компиляции шейдеров
find_package(Vulkan REQUIRED COMPONENTS shaderc_combined)
find_package(OpenMP REQUIRED)

# --- Настройка ImGui источников ---
set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

set(EXECUTABLE_NAME ${PROJECT_NAME})
file(GLOB_RECURSE SOURCES "main.cpp" "core/*.cpp" "core/*.hpp" "core/*.h")

add_executable(${EXECUTABLE_NAME} ${SOURCES} ${IMGUI_SOURCES})

target_include_directories(${EXECUTABLE_NAME} PRIVATE
        core/ core/sep
        ${IMGUI_DIR} ${IMGUI_DIR}/backends
)

# --- Оптимизации ---
if(MSVC)
    target_compile_options(${EXECUTABLE_NAME} PRIVATE $<$<CONFIG:Release>:/O2 /Oi /arch:AVX2>)
else()
    # ВНИМАНИЕ: Если Polly всё еще крашит билд, временно закомментируй эти 3 строки ниже
    target_compile_options(${EXECUTABLE_NAME} PRIVATE
            $<$<CONFIG:Release>:-O3 -march=native -ffast-math -fopenmp -flto=full -fplugin=LLVMPolly.so -mllvm -polly>
    )
    target_link_options(${EXECUTABLE_NAME} PRIVATE $<$<CONFIG:Release>:-fopenmp -flto=full -fuse-ld=mold>)
endif()

# --- Линковка ---
target_link_libraries(${EXECUTABLE_NAME} PUBLIC
        Jolt
        SDL3::SDL3-shared
        glm::glm
        fastgltf::fastgltf
        nlohmann_json::nlohmann_json
        Vulkan::Vulkan
        Vulkan::shaderc_combined # Используем современный таргет через неймспейс
        OpenMP::OpenMP_CXX
        pthread
)

if(WIN32)
    add_custom_command(TARGET ${EXECUTABLE_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:${EXECUTABLE_NAME}> $<TARGET_RUNTIME_DLLS:${EXECUTABLE_NAME}>
            COMMAND_EXPAND_LISTS
    )
endif()