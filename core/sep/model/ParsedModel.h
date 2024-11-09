#ifndef PARSEDMODEL_H
#define PARSEDMODEL_H
#include <vector>

#include "../../Logger.hpp"
#include "../graphics/Graphics.hpp"
#include "fastgltf/core.hpp"
#include "fastgltf/tools.hpp"

class ParsedModel {
    public:
    std::vector<std::vector<Graphics::Vertex>> primitives{};
    std::vector<uint16_t> indices{};

    static fastgltf::Asset loadAsset(std::filesystem::path& path) {
        Logger* LOGGER = Logger::of("ParsedModel.h");

        constexpr auto options = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble | fastgltf::Options::LoadExternalBuffers | fastgltf::Options::LoadExternalImages | fastgltf::Options::GenerateMeshIndices;
        static constexpr auto extensions = fastgltf::Extensions::KHR_mesh_quantization | fastgltf::Extensions::KHR_texture_transform | fastgltf::Extensions::KHR_materials_variants;

        fastgltf::Parser parser(extensions);
        auto buffer = fastgltf::GltfDataBuffer::FromPath(path);

        auto asset = parser.loadGltf(buffer.get(), path.parent_path(), options);

        if (asset.error() != fastgltf::Error::None) {
            LOGGER->log(Logger::severity::LOG, "Error loading model:", fastgltf::getErrorMessage(asset.error()));
        }

        return std::move(asset.get());
    }

    static std::vector<std::vector<Graphics::Vertex>> getPrimitives(fastgltf::Asset asset) {
        for (fastgltf::Mesh mesh : asset.meshes) {
            for(mesh.primitives.)
        }
        return ;
    }

    private:

    ParsedModel() {

    }
};

#endif //PARSEDMODEL_H
