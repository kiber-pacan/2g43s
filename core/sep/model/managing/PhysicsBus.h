//
// Created by down1 on 26.01.2026.
//

#ifndef INC_2G43S_PHYSICSBUS_H
#define INC_2G43S_PHYSICSBUS_H

#pragma region Include
#include <Jolt/Jolt.h>

// Jolt includes
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

#include "Logger.hpp"
#define GLM_FORCE_RADIANS
#include <imgui_impl_sdl3.h>
#include <glm/glm.hpp>

#include "ModelGroup.h"
#pragma endregion

#pragma region Generic
class Engine;

namespace Layers {
    static constexpr JPH::ObjectLayer NON_MOVING = 0;
    static constexpr JPH::ObjectLayer MOVING = 1;
    static constexpr JPH::uint NUM_LAYERS = 2;
}

class ObjectLayerPairFilterImpl final : public JPH::ObjectLayerPairFilter {
public:
    [[nodiscard]] bool ShouldCollide(const JPH::ObjectLayer firstObject, const JPH::ObjectLayer secondObject) const override {
        switch (firstObject) {
            case Layers::NON_MOVING:
                // Nonmoving objects collide only with moving ones
                return secondObject == Layers::MOVING;
            case Layers::MOVING:
                // Moving objects collides with everything
                return true;
            default:
                JPH_ASSERT(false);
                return false;
        }
    }
};

namespace BroadPhaseLayers {
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr uint NUM_LAYERS(2);
}

class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
public:
    BPLayerInterfaceImpl() {
        // Create a mapping table from object to broad phase layer
        mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
    }

    [[nodiscard]] uint GetNumBroadPhaseLayers() const override {
        return BroadPhaseLayers::NUM_LAYERS;
    }

    [[nodiscard]] JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override {
        JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
        return mObjectToBroadPhase[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    [[nodiscard]] const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override {
        switch (static_cast<JPH::BroadPhaseLayer::Type>(inLayer)) {
            case static_cast<JPH::BroadPhaseLayer::Type>(BroadPhaseLayers::NON_MOVING):
                return "NON_MOVING";
            case static_cast<JPH::BroadPhaseLayer::Type>(BroadPhaseLayers::MOVING):
                return "MOVING";
            default:
                JPH_ASSERT(false); return "INVALID";
        }
    }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
    JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

/// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl final : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    [[nodiscard]] bool ShouldCollide(JPH::ObjectLayer firstLayer, JPH::BroadPhaseLayer secondLayer) const override {
        switch (firstLayer) {
            case Layers::NON_MOVING:
                return secondLayer == BroadPhaseLayers::MOVING;
            case Layers::MOVING:
                return true;
            default:
                JPH_ASSERT(false);
                return false;
        }
    }
};

class MyContactListener final : public JPH::ContactListener {
    Logger LOGGER = Logger("ContactListener");
public:
    // See: ContactListener
    JPH::ValidateResult	OnContactValidate(const JPH::Body &inBody1, const JPH::Body &inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult &inCollisionResult) override {
        //LOGGER.info("Contact validate callback");

        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    void OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override {
        //LOGGER.info("A contact was added");
    }

    void OnContactPersisted(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override {
        //LOGGER.info("A contact was persisted");
    }

    void OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair) override {
        //LOGGER.info("A contact was removed");
    }
};

// An example activation listener
class MyBodyActivationListener final : public JPH::BodyActivationListener {
    Logger LOGGER = Logger("BodyActivationListener");
public:
    void OnBodyActivated(const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData) override {
        //LOGGER.info("A body got activated");
    }

    void OnBodyDeactivated(const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData) override {
        //LOGGER.info("A body went to sleep");
    }
};

#pragma endregion



struct PhysicsBus {
    #pragma region Parameters
    uint ALLOCATOR_SIZE = 10 * 1024 * 1024; // 10 MB
    uint MAX_JOBS = 4096;
    uint MAX_BARRIERS = 16;

    uint TICK_RATE = 256;
    float cDeltaTime = 1.0f / TICK_RATE;
    #pragma endregion

    std::vector<JPH::BodyID> physicsBodies{};

    JPH::TempAllocatorImpl* temp_allocator{};
    JPH::JobSystemThreadPool* job_system{};
    JPH::PhysicsSystem* physics_system{};

    BPLayerInterfaceImpl* bp_interface{};
    ObjectVsBroadPhaseLayerFilterImpl* obj_bp_filter{};
    ObjectLayerPairFilterImpl* obj_obj_filter{};

    MyBodyActivationListener* body_activation_listener{};
    MyContactListener* contact_listener{};

    std::unordered_map<std::string, JPH::ShapeRefC> collisionHulls{};
    std::unordered_map<std::string, JPH::ShapeRefC> collisionMeshes{};

    static glm::mat4 JoltToGlm(const JPH::RMat44 &joltMatrix);

    #pragma region Main
    void initializeJPH();

    void iterateJPH(Engine& engine) const;

    void shutdownJPH() const;
    #pragma endregion



    #pragma region Body
    JPH::BodyID createBody(JPH::BodyCreationSettings body_settings) const;
    #pragma endregion
};


#endif //INC_2G43S_PHYSICSBUS_H