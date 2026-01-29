//
// Created by down1 on 29.01.2026.
//
#include "ModelEntityManager.hpp"

#include "ModelBus.hpp"

#pragma region buffers
VkDeviceSize ModelEntityManager::getIndexBufferSize() const {
    VkDeviceSize bufferSize = 0;
    for (const auto &model : std::views::transform(std::views::values(groups), &ModelGroup::model)) {
        bufferSize += sizeof(model->indices[0]) * model->indices.size();
    }

    return bufferSize;
}

VkDeviceSize ModelEntityManager::getVertexBufferSize() const {
    VkDeviceSize bufferSize = 0;
    for (const auto& model : std::views::transform(std::views::values(groups), &ModelGroup::model)) {
        for (const auto& mesh: model->meshes) {
            bufferSize += sizeof(mesh[0]) * mesh.size();
        }
    }

    return bufferSize;
}

VkDeviceSize ModelEntityManager::getModelBufferSize() const {
    VkDeviceSize bufferSize = 0;
    for (const auto& instance : std::views::transform(std::views::values(groups), &ModelGroup::instances)) {
        bufferSize += sizeof(instance[0]) * instance.size();
    }

    return bufferSize;
}

VkDeviceSize ModelEntityManager::getModelDataBufferSize() const {
    VkDeviceSize bufferSize = 0;
    for (const auto& instance : std::views::transform(std::views::values(groups), &ModelGroup::instances)) {
        bufferSize += sizeof(glm::vec4) * instance.size() * 3;
    }

    return bufferSize;
}
#pragma endregion

#pragma region data
// Model loading methods
std::vector<uint32_t> ModelEntityManager::getAllIndices() const {
    std::vector<uint32_t> indices{};
    for (const auto& model : std::views::transform(std::views::values(groups), &ModelGroup::model)) {
        #if __cpp_lib_containers_ranges >= 202202L
        indices.append_range(model->indices);
        #else
        indices.insert(indices.end(), model->indices.begin(), model->indices.end());
        #endif
    }

    return indices;
}

std::vector<uint32_t> ModelEntityManager::getIndices(const std::string& file) const {
    if (const auto& it = groups.find(file); it != groups.end()) {
        return it->second.model->indices;
    }
    return {};
}


std::vector<Vertex> ModelEntityManager::getAllVertices() const {
    std::vector<Vertex> vertices{};

    for (const auto& model : std::views::transform(std::views::values(groups), &ModelGroup::model)) {
        for (const auto& mesh: model->meshes) {
            #if __cpp_lib_containers_ranges >= 202202L
            vertices.append_range(mesh);
            #else
            vertices.insert(vertices.end(), mesh.begin(), mesh.end());
            #endif
        }
    }

    return vertices;
}

std::vector<Vertex> ModelEntityManager::getVertices(const std::string& file) const {
    std::vector<Vertex> vertices{};

    if (const auto& it = groups.find(file); it != groups.end()) {
        const auto& meshes = it->second.model->meshes;
        std::for_each(meshes.begin(), meshes.end(), [&vertices](const auto& mesh) {
            #if __cpp_lib_containers_ranges >= 202202L
            vertices.append_range(mesh);
            #else
            vertices.insert(vertices.end(), mesh.begin(), mesh.end());
            #endif
        });
    }

    return vertices;
}
#pragma endregion

#pragma region count
size_t ModelEntityManager::modelsCount() const {
    return groups.size();
}


size_t ModelEntityManager::getInstanceCount(const std::string& file) const {
    if (const auto& it = groups.find(file); it != groups.end()) {
        return it->second.instances.size();
    }
    return 0;
}

size_t ModelEntityManager::getTotalInstanceCount() const {
    const auto& instances = std::views::transform(std::views::values(groups), &ModelGroup::instances);

    return std::accumulate(instances.begin(), instances.end(), 0, [](uint32_t sum, const auto& instance) {
        return sum + instance.size();
    });
}

size_t ModelEntityManager::getTotalModelCount() const {
    return groups.size();
}


uint32_t ModelEntityManager::getIndexCount(const std::string& name) const {
    return groups.at(name).model->indices.size();
}

uint32_t ModelEntityManager::getIndexCount(const std::shared_ptr<ParsedModel>& model) {
    return model->indices.size();
}


int32_t ModelEntityManager::getVertexCount(const std::string& name) const {
    const auto& meshes = groups.at(name).model->meshes;

    return std::accumulate(meshes.begin(), meshes.end(), 0, [](uint32_t sum, const auto& mesh) {
        return sum + mesh.size();
    });
}

int32_t ModelEntityManager::getVertexCount(const std::shared_ptr<ParsedModel>& model) {
    return std::accumulate(model->meshes.begin(), model->meshes.end(), 0, [](uint32_t sum, const auto& mesh) {
        return sum + mesh.size();
    });
}
#pragma endregion

JPH::ShapeRefC ModelEntityManager::physShape(const std::string& file) {
    JPH::ShapeRefC collisionShape;

    if (const auto it = physicsBus.collisionHulls.find(file); it != physicsBus.collisionMeshes.end()) {
        collisionShape = it->second;
    } else {
        collisionShape = groups[file].model.get()->createJoltConvexHull();
        physicsBus.collisionHulls.insert({file, collisionShape});
    }

    return collisionShape;
}

JPH::ShapeRefC ModelEntityManager::staticShape(const std::string& file) {
    JPH::ShapeRefC collisionShape;

    if (const auto it = physicsBus.collisionMeshes.find(file); it != physicsBus.collisionMeshes.end()) {
        collisionShape = it->second;
    } else {
        collisionShape = groups[file].model.get()->createJoltMesh();
        physicsBus.collisionMeshes.insert({file, collisionShape});
    }

    return collisionShape;
}


void ModelEntityManager::addBody(const JPH::BodyID id, std::string file, size_t index) {
    std::lock_guard<std::mutex> lock(bodyID_mutex);
    bodyID[id] = {file, index};
}


void ModelEntityManager::randomVolume(const std::string& file, size_t count, const float mass, const glm::vec3& min, const glm::vec3& max) {
    auto& group = groups[file];
    group.instances.resize(count);

    JPH::BodyCreationSettings settings(physShape(file), JPH::RVec3(0, 0, 0), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::MOVING);
    settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    settings.mMassPropertiesOverride.mMass = mass;
    settings.mLinearDamping = 0.05f;
    settings.mAngularDamping = 0.05f;

    #pragma omp parallel for schedule(static) default(none) shared(settings, count, group, min, max, file)
    for (int x = 0; x < count; ++x) {
        const glm::vec4 pos(Random::randomNum_T(min.x, max.x), Random::randomNum_T(min.y, max.y), Random::randomNum_T(min.z, max.z), 0);
        group.instances[x] = ModelInstance(group.model, pos);
        settings.mPosition.Set(pos.x, pos.y, pos.z);
        addBody(physicsBus.createBody(settings), file, x);
    }
}

void ModelEntityManager::square(const std::string& file, size_t count, const double gap) {
    auto& group = groups[file];
    group.instances.resize(count * count);

    JPH::BodyCreationSettings settings(physShape(file), JPH::RVec3(0, 0, 0), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::MOVING);
    settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    settings.mMassPropertiesOverride.mMass = 35;
    settings.mLinearDamping = 0.05f;
    settings.mAngularDamping = 0.05f;

    #pragma omp parallel for schedule(static) default(none) shared(settings, count, group, gap, file)
    for (int x = 0; x < count; ++x) {
        for (int y = 0; y < count; ++y) {
            const glm::vec4 pos(x * gap, y * gap, 0, 0);

            group.instances[x * count + y] = ModelInstance(group.model, pos);
            settings.mPosition.Set(pos.x, pos.y, pos.z);
            addBody(physicsBus.createBody(settings), file, x * count + y);
        }
    }
}

void ModelEntityManager::staticInstance(const std::string& file, const glm::vec4 pos) {
    JPH::BodyCreationSettings settings(staticShape(file), JPH::RVec3(pos.x, pos.y, pos.z), JPH::Quat::sIdentity(), JPH::EMotionType::Static, Layers::NON_MOVING);
    settings.mLinearDamping = 0.05f;
    settings.mAngularDamping = 0.05f;
    settings.mFriction = 0.6f;
    instance(settings, file, pos);
}

void ModelEntityManager::physicsInstance(const std::string& file, const glm::vec4 pos) {
    JPH::BodyCreationSettings settings(physShape(file), JPH::RVec3(pos.x, pos.y, pos.z), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::MOVING);
    settings.mLinearDamping = 0.05f;
    settings.mAngularDamping = 0.05f;
    settings.mFriction = 0.6f;
    instance(settings, file, pos);
}

#pragma region PhysicsInstance
void ModelEntityManager::physicsInstance(const std::string& file, const glm::vec4 pos, const float kg) {
    JPH::BodyCreationSettings settings(physShape(file), JPH::RVec3(pos.x, pos.y, pos.z), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::MOVING);
    settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    settings.mMassPropertiesOverride.mMass = kg;
    settings.mLinearDamping = 0.05f;
    settings.mAngularDamping = 0.05f;
    settings.mFriction = 0.6f;
    instance(settings, file, pos);
}

void ModelEntityManager::physicsInstance(const std::string& file, const glm::vec4 pos, const glm::vec3 impulse) {
    JPH::BodyCreationSettings settings(physShape(file), JPH::RVec3(pos.x, pos.y, pos.z), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::MOVING);
    settings.mLinearDamping = 0.05f;
    settings.mAngularDamping = 0.05f;
    settings.mFriction = 0.6f;
    instance(settings, file, impulse, pos);
}

void ModelEntityManager::physicsInstance(const std::string& file, const glm::vec4 pos, const float kg, const glm::vec3 impulse) {
    JPH::BodyCreationSettings settings(physShape(file), JPH::RVec3(pos.x, pos.y, pos.z), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::MOVING);
    settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    settings.mMassPropertiesOverride.mMass = kg;
    settings.mLinearDamping = 0.05f;
    settings.mAngularDamping = 0.05f;
    settings.mFriction = 0.6f;
    instance(settings, file, impulse, pos);
}

#pragma endregion

void ModelEntityManager::scene() {
    const auto start1 = std::chrono::high_resolution_clock::now();
    indexedFiles = {"box_opt.glb", "land2_opt.glb"};
    loadModels("/home/down1/2g43s/core/models/");
    //ModelBus::loadModels(groups, "/home/down1/2g43s/core/models/", "box_opt.glb", "land2_opt.glb");
    const auto end1 = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> duration1 = end1 - start1;

    LOGGER.success("LOADED ${} models in ${} seconds!", getTotalModelCount(), duration1.count());

    const auto start = std::chrono::high_resolution_clock::now();

    staticInstance("land2_opt.glb", glm::vec4(0, 0, -4, 1));
    physicsInstance("box_opt.glb", glm::vec4(0, 0, 7, 1), 32);
    //randomVolume("box_opt.glb", 64, 32, glm::vec3(16, 16, 16), glm::vec3(48, 48, 32));
    //square("box_opt.glb", 10, 1);


    const auto end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> duration = end - start;

    LOGGER.success("LOADED ${} instances in ${} seconds!", getTotalInstanceCount(), duration.count());
}