#ifndef PARSEDMODEL_H
#define PARSEDMODEL_H
#include <vector>

#include "../../Logger.hpp"
#include "../graphics/Graphics.hpp"
#include "fastgltf/core.hpp"
#include "fastgltf/tools.hpp"

class ParsedModel {
    public:
    std::vector<std::vector<Graphics::Vertex>> meshes{};
    std::vector<uint16_t> indices{};

    ParsedModel(std::filesystem::path& path) {
        //loadModel(loadAsset(path));
    }

    fastgltf::Asset loadAsset(std::filesystem::path& path) {
        Logger* LOGGER = Logger::of("ParsedModel.h");

        constexpr auto options = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble | fastgltf::Options::LoadExternalBuffers | fastgltf::Options::LoadExternalImages | fastgltf::Options::GenerateMeshIndices;
        static constexpr auto extensions = fastgltf::Extensions::KHR_mesh_quantization | fastgltf::Extensions::KHR_texture_transform | fastgltf::Extensions::KHR_materials_variants;

        fastgltf::Parser parser(extensions);
        auto buffer = fastgltf::GltfDataBuffer::FromPath(path);

        if (!bool(buffer)) {
            LOGGER->error("Failed to open glTF file: ${}", fastgltf::getErrorMessage(buffer.error()));
        }

        auto asset = parser.loadGltf(buffer.get(), path.parent_path(), options);

        if (asset.error() != fastgltf::Error::None) {
            LOGGER->error("Error loading model:", fastgltf::getErrorMessage(asset.error()));
        }

        return std::move(asset.get());
    }

    void loadModel(fastgltf::Asset asset) {
        for (fastgltf::Mesh mesh : asset.meshes) {
            for (fastgltf::Primitive primitive : mesh.primitives) {
                auto* position = primitive.findAttribute("POSITION");
                auto& positionAccessor = asset.accessors[position->accessorIndex];

                std::vector<Graphics::Vertex> tempMesh{};
                std::vector<std::uint16_t> tempIndices{};

                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, positionAccessor, [&](fastgltf::math::fvec3 pos, std::size_t idx) {
                    tempMesh[idx] = { {pos[0], pos[1], pos[2]}, {1, 1, 1}, {fastgltf::math::fvec2()[0], fastgltf::math::fvec2()[1]} };
                });

                this->meshes.emplace_back(tempMesh);

                if (primitive.indicesAccessor.has_value()) {
                    auto& accessor = asset.accessors[primitive.indicesAccessor.value()];
                    tempIndices.resize(accessor.count);

                    std::size_t idx = 0;
                    fastgltf::iterateAccessor<std::uint32_t>(asset, accessor, [&](std::uint32_t index) {
                        tempIndices[idx++] = index;
                    });
                }

                this->indices.insert(indices.end(), tempIndices.begin(), tempIndices.end());
            }
        }
    }
};

#endif //PARSEDMODEL_H
