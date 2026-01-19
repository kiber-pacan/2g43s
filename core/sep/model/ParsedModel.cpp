#include "ParsedModel.hpp"

#include <numeric>
#include <ranges>

#include "Images.hpp"
#include "Logger.hpp"
#include <stb_image.h>
#include "fastgltf/tools.hpp"


fastgltf::Asset ParsedModel::loadAsset(const std::filesystem::path& path) {
     auto LOGGER = Logger("ParsedModel.h");

     constexpr auto options = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble | fastgltf::Options::LoadExternalBuffers | fastgltf::Options::LoadExternalImages | fastgltf::Options::GenerateMeshIndices;
     static constexpr auto extensions = fastgltf::Extensions::KHR_mesh_quantization | fastgltf::Extensions::KHR_texture_transform | fastgltf::Extensions::KHR_materials_variants | fastgltf::Extensions::KHR_materials_specular;

     fastgltf::Parser parser(extensions);
     auto buffer = fastgltf::GltfDataBuffer::FromPath(path);

     if (!static_cast<bool>(buffer)) {
         LOGGER.error("Failed to open glTF file: ${}", fastgltf::getErrorMessage(buffer.error()));
     }

     auto asset = parser.loadGltf(buffer.get(), path.parent_path(), options);

     if (asset.error() != fastgltf::Error::None) {
         LOGGER.error("Error loading model: ${}", fastgltf::getErrorMessage(asset.error()));
     }

     return std::move(asset.get());
 }

void ParsedModel::loadModel(fastgltf::Asset asset) {
    auto lastIndex = -1;

    std::vector<uint32_t> primitiveSizes{};
    primitiveSizes.resize(asset.meshes.size());

    for (int i = 0; i < asset.meshes.size(); ++i) {
        primitiveSizes[i] = asset.meshes[i].primitives.size();
    }

    std::vector<uint32_t> indicesCount{};
    indicesCount.resize(std::reduce(primitiveSizes.begin(), primitiveSizes.end()) + 1); // + 1 for zero index
    indicesCount[0] = 0;

    uint32_t idx = 1;
    for (int i = 0; i < primitiveSizes.size(); ++i) {
        for (int i1 = 0; i1 < primitiveSizes[i]; ++i1) {
            indicesCount[idx++] = asset.accessors[*asset.meshes[i].primitives[i1].indicesAccessor].count + indicesCount[glm::max<int>(idx - 1, 0)];
        }
    }

    this->indices.resize(indicesCount.back());

    uint32_t globalIndex = 0;
    for (int i = 0; i < primitiveSizes.size(); ++i) {
        for (int i1 = 0; i1 < primitiveSizes[i]; ++i1) {
            const auto& primitive = asset.meshes[i].primitives[i1];

            // POS
            auto* position = primitive.findAttribute("POSITION");
            auto& positionAccessor = asset.accessors[position->accessorIndex];

            std::vector<Vertex> tempMesh(positionAccessor.count);

            fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, positionAccessor, [&](fastgltf::math::fvec3 pos, std::size_t idx) {
                tempMesh[idx] = {
                    {pos[0], pos[1], pos[2]},
                    {1, 1, 1},
                    {fastgltf::math::fvec2()[0],fastgltf::math::fvec2()[1]}
                };
            });

            // UV
            auto* texcoord = primitive.findAttribute("TEXCOORD_0");
            if (texcoord != nullptr) {
                auto& texcoordAccessor = asset.accessors[texcoord->accessorIndex];

                if (texcoordAccessor.type == fastgltf::AccessorType::Vec2) {
                    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(
                        asset, texcoordAccessor,
                         [&](fastgltf::math::fvec2 uv, std::size_t idx) {
                             tempMesh[idx].texCoord = { uv[0], uv[1] };
                         }
                    );
                }
            }

            // INDICES
            uint32_t baseVertex = 0;
            for (auto& m : this->meshes) {
                baseVertex += static_cast<uint32_t>(m.size());
            }

            if (primitive.indicesAccessor.has_value()) {
                auto& accessor = asset.accessors[primitive.indicesAccessor.value()];

                uint32_t idx = 0;
                fastgltf::iterateAccessor<std::uint32_t>(asset, accessor, [&](const std::uint32_t index) {
                    this->indices[indicesCount[globalIndex] + idx] = index + baseVertex;
                    idx++;
                });

                globalIndex++;
            }

            this->meshes.emplace_back(std::move(tempMesh));


            if (primitive.materialIndex.has_value()) {
                const auto start = std::chrono::high_resolution_clock::now();

                auto& accessor = asset.materials[primitive.materialIndex.value()];

                if (accessor.pbrData.baseColorTexture.has_value()) {
                    const auto& texture = asset.textures[accessor.pbrData.baseColorTexture.value().textureIndex];

                    if (texture.imageIndex.has_value() && lastIndex != texture.imageIndex.value()) {
                        auto& image = asset.images[texture.imageIndex.value()];
                        processImageData(image, asset);

                        this->textures.back().indexOffset = baseVertex;

                        lastIndex = texture.imageIndex.value();

                        const auto end = std::chrono::high_resolution_clock::now();
                        const std::chrono::duration<double> duration = end - start;

                        std::cout << "Loaded texture in: " << duration.count() << std::endl;
                    }
                }
            }
        }
    }
}


void ParsedModel::processImageData(fastgltf::Image& image, fastgltf::Asset& asset) {
    std::visit(fastgltf::visitor {
        [](auto& arg) {},
        [&](fastgltf::sources::BufferView& view) {
            auto& bufferView = asset.bufferViews[view.bufferViewIndex];
            auto& buffer = asset.buffers[bufferView.bufferIndex];

            std::visit(fastgltf::visitor {
                [&](fastgltf::sources::Array& array) {
                    const uint8_t* startPtr = reinterpret_cast<const uint8_t*>(array.bytes.data()) + bufferView.byteOffset;

                    Texture texture{};

                    texture.pixels = stbi_load_from_memory(startPtr, buffer.byteLength, &texture.texWidth, &texture.texHeight, &texture.texChannels, STBI_rgb_alpha);

                    textures.emplace_back(std::move(texture));
                },
                [](auto&) {}
            }, buffer.data);
        },
    }, image.data);

}


void ParsedModel::calcOcclusionSphere() {
    std::vector<Vertex> vertices;
    #if __cpp_lib_containers_ranges >= 202202L
    vertices.append_range(std::views::join(meshes));
    #else
    indices.insert(indices.end(), model->indices.begin(), model->indices.end());
    #endif


    glm::vec3 modelCenter(0.0f);
    for (const Vertex& p : vertices) {
        modelCenter += p.pos;
    }
    modelCenter /= static_cast<float>(vertices.size());

    float radius = 0.0f;
    for (const Vertex& p : vertices) {
        radius = std::max(radius, glm::length(p.pos - modelCenter));
    }

    sphere = glm::vec4(modelCenter, radius);

}