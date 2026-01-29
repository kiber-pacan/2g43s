//
// Created by down1 on 09.01.2026.
//

#ifndef INC_2G43S_TEXTUREINDEXOBJECT_H
#define INC_2G43S_TEXTUREINDEXOBJECT_H
#include <cstdint>
#include <vector>

struct alignas(16) Index {
    uint32_t firstIndex;
    uint32_t indexCount;
    uint32_t pad1;
    uint32_t pad2;
};


struct TextureIndexBufferObject {
    std::vector<Index> indices;
};
#endif //INC_2G43S_TEXTUREINDEXOBJECT_H