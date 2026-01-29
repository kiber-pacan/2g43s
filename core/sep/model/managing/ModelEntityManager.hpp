//
// Created by down1 on 29.01.2026.
//

#ifndef INC_2G43S_MODELENTITYMANAGER_H
#define INC_2G43S_MODELENTITYMANAGER_H
#include <map>
#include <memory>
#include <vector>
#include <ranges>

#include "PhysicsBus.h"
#include "ModelInstance.hpp"
#include "ParsedModel.hpp"
#include "Images.hpp"
#include "ModelGroup.h"
#include "Random.hpp"

struct ModelEntityManager {
    std::vector<std::string> indexedFiles{};
    std::unordered_map<std::string, ModelGroup> groups{};
    std::unordered_map<JPH::BodyID, std::pair<std::string, size_t>> bodyID{};

    std::mutex bodyID_mutex;
    std::array<bool, 4> dirty{true, true, true, true};

    PhysicsBus physicsBus{};
    Logger LOGGER{"MEM"};

    ModelEntityManager() = default;

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



    uint32_t getIndexCount(const std::string& name) const;

    static uint32_t getIndexCount(const std::shared_ptr<ParsedModel>& model);


    int32_t getVertexCount(const std::string& name) const;

    static int32_t getVertexCount(const std::shared_ptr<ParsedModel>& model);
    #pragma endregion

    JPH::ShapeRefC physShape(const std::string& file);

    JPH::ShapeRefC staticShape(const std::string& file);

    void addBody(JPH::BodyID id, std::string file, size_t index);

    void randomVolume(const std::string& file, size_t count, float mass, const glm::vec3& min, const glm::vec3& max);

    void square(const std::string& file, size_t count, double gap);

    /// TIP args: pos, rot, scl (MAX 3 ELEMENTS)
    void instance(const JPH::BodyCreationSettings body_settings, const std::string& file, const glm::vec3 impulse, const auto&... args) {
        static_assert(sizeof...(args) <= 3, "Maximum 3 arguments allowed!");

        if (groups.contains(file)) {
            groups[file].instances.emplace_back(groups[file].model, args...);
            dirty.fill(true);

            const auto id = physicsBus.createBody(body_settings);
            addBody(id, file, groups[file].instances.size() - 1);

            JPH::BodyInterface &body_interface = physicsBus.physics_system->GetBodyInterface();
            body_interface.SetLinearVelocity(id, JPH::Vec3(impulse.x, impulse.y, impulse.z));
        } else {
            LOGGER.error("File ${} does not exist", file);
        }
    }

    void instance(const JPH::BodyCreationSettings body_settings, const std::string& file, const auto&... args) {
        instance(body_settings, file, glm::vec3(0), args...);
    }


    void loadModels(const std::string& location) {
        for (const auto& file : indexedFiles) {
            groups.insert(
                {file, ModelGroup(std::make_shared<ParsedModel>(location + file))}
                );
        }
    }

    void staticInstance(const std::string& file, glm::vec4 pos);

    void physicsInstance(const std::string& file, glm::vec4 pos);

    void physicsInstance(const std::string& file, glm::vec4 pos, float kg);

    void physicsInstance(const std::string& file, glm::vec4 pos, glm::vec3 impulse);

    void physicsInstance(const std::string& file, glm::vec4 pos, float kg, glm::vec3 impulse);

    void scene();
};

#endif //INC_2G43S_MODELENTITYMANAGER_H