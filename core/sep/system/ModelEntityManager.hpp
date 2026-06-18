//
// Created by down1 on 29.01.2026.
//

#ifndef INC_2G43S_MODELENTITYMANAGER_H
#define INC_2G43S_MODELENTITYMANAGER_H
#include <map>
#include <memory>
#include <vector>
#include <ranges>

#include "PhysicsBus.hpp"
#include "ModelInstance.hpp"
#include "ParsedModel.hpp"
#include "ModelGroup.hpp"
#include "ModelRegion.h"
#include "Random.hpp"

struct ModelEntityManager {
    ModelEntityManager() = default;

    std::vector<std::string> indexedFiles{};
    std::vector<ModelGroup> groups{};

    std::unordered_map<std::string, size_t> indices{};
    std::unordered_map<JPH::BodyID, std::pair<std::string, size_t>> bodyID{};

    std::vector<ModelRegion> regions{};
    std::vector<std::vector<size_t>> modelRegions{};

    std::mutex bodyID_mutex;
    std::array<bool, 4> dirty{true, true, true, true};

    PhysicsBus physicsBus{};
    Logger LOGGER{"ModelEntityManager"};

    #pragma region sizes
    VkDeviceSize getIndexBufferSize() const;

    VkDeviceSize getVertexBufferSize() const;

    VkDeviceSize getModelBufferSize() const;

    VkDeviceSize getModelDataBufferSize() const;
    #pragma endregion

    #pragma region data
    // Model loading methods
    std::vector<uint32_t> getAllIndices() const;

    std::vector<uint32_t> getIndices(const std::string& file) const;


    std::vector<Vertex> getAllVertices() const;

    std::vector<Vertex> getVertices(const std::string& file) const;
    #pragma endregion

    #pragma region count
    size_t modelsCount() const;

    size_t getInstanceCount(const std::string& file) const;

    size_t getTotalInstanceCount() const;

    size_t getTotalModelCount() const;



    uint32_t getIndexCount(const std::string& name);

    static uint32_t getIndexCount(const std::shared_ptr<ParsedModel>& model);


    int32_t getVertexCount(const std::string& name);

    static int32_t getVertexCount(const std::shared_ptr<ParsedModel>& model);
    #pragma endregion

    JPH::ShapeRefC physShape(const std::string& file);

    JPH::ShapeRefC staticShape(const std::string& file);

    void addBody(JPH::BodyID id, const std::string& file, size_t index);

    void randomVolume(const std::string& file, size_t count, float mass, const glm::vec3& min, const glm::vec3& max);

    void square(const std::string& file, size_t count, double gap);

    /// TIP args: pos, rot, scl (MAX 3 ELEMENTS)
    void instance(const JPH::BodyCreationSettings& settings, const std::string& file, const glm::vec3 impulse, const auto&... args) {
        static_assert(sizeof...(args) <= 3, "Maximum 3 arguments allowed!");

        if (indices.contains(file)) {
            groups[indices[file]].instances.emplace_back(groups[indices[file]].model, args...);
            dirty.fill(true);

            const auto id = physicsBus.createBody(settings);
            addBody(id, file, groups[indices[file]].instances.size() - 1);

            JPH::BodyInterface &body_interface = physicsBus.physics_system->GetBodyInterface();
            body_interface.SetLinearVelocity(id, JPH::Vec3(impulse.x, impulse.y, impulse.z));
        } else {
            LOGGER.error("File ${} does not exist", file);
        }
    }

    void instance(const JPH::BodyCreationSettings& settings, const std::string& file, const auto&... args) {
        instance(settings, file, glm::vec3(0), args...);
    }


    void loadModels(const std::string& location) {
        static uint32_t index = 0;
        for (const auto& file : indexedFiles) {
            regions.emplace_back(index);
            modelRegions.emplace_back(index);

            indices.insert({file, index++});
            groups.emplace_back(std::make_shared<ParsedModel>(std::string{PROJECT_ROOT} + location + file));
        }
        index = 0;
    }

    void staticInstance(const std::string& file, glm::vec4 pos);

    void physicsInstance(const std::string& file, glm::vec4 pos);

    void physicsInstance(const std::string& file, glm::vec4 pos, float kg);

    void physicsInstance(const std::string& file, glm::vec4 pos, glm::vec3 impulse);

    void physicsInstance(const std::string& file, glm::vec4 pos, float kg, glm::vec3 impulse);

    void scene();
};

#endif //INC_2G43S_MODELENTITYMANAGER_H
