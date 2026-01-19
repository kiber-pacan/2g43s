//
// Created by down1 on 19.10.2025.
//

#ifndef INC_2G43S_MODELCULLINGBUFFEROBJECT_H
#define INC_2G43S_MODELCULLINGBUFFEROBJECT_H
#include <vector>

struct CullingData {
    CullingData(const glm::vec4 sphere, const uint32_t drawCommandIndex) : sphere(sphere), drawCommandIndex(drawCommandIndex) {}
    glm::vec4 sphere;
    uint32_t drawCommandIndex;
    uint32_t _pad[3]{};
};

struct ModelCullingBufferObject {
    std::vector<CullingData> cullingDatas;
};



#endif //INC_2G43S_MODELCULLINGBUFFEROBJECT_H