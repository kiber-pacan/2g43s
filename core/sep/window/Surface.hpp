#ifndef SURFACE_HPP
#define SURFACE_HPP

//#include "core/logger.hpp"
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include "../util/Logger.hpp"

struct Surface {
    // Creating surface with SDL3
    static void createSurface(VkSurfaceKHR& surface, SDL_Window* window, VkInstance& instance) {
        auto LOGGER = Logger("surface.hpp");
        if (SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface) == true) {
            LOGGER.success("Successfuly CREATED Vulkan surface with SDL3");
        } else {
            LOGGER.error("FAILED to CREATE Vulkan surface");
        }
    }
};

#endif //SURFACE_HPP
