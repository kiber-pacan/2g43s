//
// Created by down1 on 26.02.2026.
//

#ifndef INC_2G43S_TYPES_H
#define INC_2G43S_TYPES_H

#include "glmMath.h"
#include "vulkan/vulkan_core.h"

struct CullingData {
    CullingData(const glm::vec4 sphere, const uint32_t drawCommandIndex) : sphere(sphere), drawCommandIndex(drawCommandIndex) {}
    glm::vec4 sphere;
    uint32_t drawCommandIndex;
    uint32_t _pad[3]{};
};

struct matrixCullingBuffer {
    std::vector<CullingData> cullingDatas;
};



struct VisibleIndicesBuffer {
    std::vector<uint32_t> vi;
};



struct alignas(16) Index {
    uint32_t firstIndex;
    uint32_t indexCount;
    uint32_t globalVertexOffset;
    uint32_t pad2;
};

struct TextureIndexBuffer {
    std::vector<Index> indices;
};



struct TextureIndexOffsetBuffer {
    std::vector<uint32_t> indexOffsets;
};



struct UniformBuffer {
    glm::mat4 view;
    glm::mat4 proj;
    uint32_t count;
};



struct UniformCullingBuffer {
    glm::vec4 planes[6];
    uint32_t totalObjects;
};



struct UniformPostprocessingBuffer {
    glm::vec2 resolution;
    float time = 0;
    float padding;
};



struct AtomicCounterBuffer {
    uint32_t counter;
};



struct DrawCommandsBuffer {
    std::vector<VkDrawIndexedIndirectCommand> commands{};
};



struct MatrixBufferObject {
    std::vector<glm::mat4> models;
};



struct MatrixDataBufferObject {
    std::vector<glm::vec4> pos;
    std::vector<glm::vec4> rot;
    std::vector<glm::vec4> scl;
    std::vector<glm::vec4> sphere;
    bool dirty2 = true;
    size_t frame = 0;
};

#endif //INC_2G43S_TYPES_H