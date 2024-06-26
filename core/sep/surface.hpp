#ifndef SURFACE_H
#define SURFACE_H

//#include "core/logger.hpp"
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

struct surface {
    static void createSurface(VkSurfaceKHR& surface, SDL_Window* window, VkInstance& instance) {
        logger* LOGGER = logger::of("SURFACE");
        if (SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface) == SDL_FALSE) {
            LOGGER->log(logger::severity::ERROR, "Failed to create SDL surface", nullptr);
        } else {
            LOGGER->log(logger::severity::SUCCESS, "Successfuly created SDL surface", nullptr);
        }
    }
};

#endif //SURFACE_H
