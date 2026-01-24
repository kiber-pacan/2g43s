#include "ParsedModel.hpp"



inline static std::unordered_map<fastgltf::MimeType, std::string> mimeTypes = {
    {fastgltf::MimeType::None, "none"},
    {fastgltf::MimeType::JPEG, "jpeg"},
    {fastgltf::MimeType::PNG, "png"},
    {fastgltf::MimeType::KTX2, "ktx2"},
    {fastgltf::MimeType::DDS, "dds"},
    {fastgltf::MimeType::GltfBuffer, "gltfBuffer"},
    {fastgltf::MimeType::OctetStream, "octecStream"},
    {fastgltf::MimeType::WEBP, "webp"},
};

void ParsedModel::loadModel(const std::filesystem::path& path) {
    auto LOGGER = Logger("loadModel()");

    // Basically all the necessary options and extensions
    static constexpr auto options =
        fastgltf::Options::DontRequireValidAssetMember |
        fastgltf::Options::AllowDouble |
        fastgltf::Options::LoadExternalBuffers |
        fastgltf::Options::LoadExternalImages |
        fastgltf::Options::GenerateMeshIndices |
        fastgltf::Options::DecomposeNodeMatrices;

    static constexpr auto extensions =
        fastgltf::Extensions::KHR_texture_basisu |
        fastgltf::Extensions::EXT_meshopt_compression |
        fastgltf::Extensions::KHR_mesh_quantization |
        fastgltf::Extensions::KHR_texture_transform |
        fastgltf::Extensions::KHR_materials_variants |
        fastgltf::Extensions::KHR_materials_specular;

    fastgltf::Parser parser(extensions);
    auto buffer = fastgltf::GltfDataBuffer::FromPath(path); // Trying to LOAD model FILE

    if (!static_cast<bool>(buffer)) {
        LOGGER.error("Failed to open glTF file: ${}", fastgltf::getErrorMessage(buffer.error()));
    }

    auto raw_asset = parser.loadGltf(buffer.get(), path.parent_path(), options); // Trying to PARSE model FILE

    if (raw_asset.error() != fastgltf::Error::None) {
        LOGGER.error("Error loading model: ${}", fastgltf::getErrorMessage(raw_asset.error()));
    }

    auto& asset = raw_asset.get(); // Getting final gltf asset after parsing model

    // Dirty hacks to make things run in parallel
    #pragma region Hacks
    std::vector<uint32_t> primitiveSizes{};
    primitiveSizes.resize(asset.meshes.size());

    for (int i = 0; i < asset.meshes.size(); ++i) {
        primitiveSizes[i] = asset.meshes[i].primitives.size();
    }

    std::vector<uint32_t> indicesCount{};
    indicesCount.emplace_back(0);
    indicesCount.resize(std::reduce(primitiveSizes.begin(), primitiveSizes.end()) + 1); // + 1 for zero index

    std::vector<uint32_t> baseVertices{};
    baseVertices.emplace_back(0);

    uint32_t idx = 1;
    for (int i = 0; i < primitiveSizes.size(); ++i) {
        for (int i1 = 0; i1 < primitiveSizes[i]; ++i1) {
            const auto& primitive = asset.meshes[i].primitives[i1];
            baseVertices.emplace_back(baseVertices.back() + asset.accessors[primitive.findAttribute("POSITION")->accessorIndex].count);
            indicesCount[idx++] = asset.accessors[*primitive.indicesAccessor].count + indicesCount[glm::max<int>(idx - 1, 0)];  // Narrowing from uint_32t to int so we wont have to deal with crazy numbers if idx == 0
        }
    }
    #pragma endregion

    this->indices.resize(indicesCount.back());

    std::vector<std::pair<size_t, size_t>> textureIndices{};
    std::vector<size_t> globalIndices{};

    double totalGeometryTime = 0;
    uint32_t globalIndex = 0;
    auto lastIndex = -1;
    for (int i = 0; i < primitiveSizes.size(); ++i) {
        for (int i1 = 0; i1 < primitiveSizes[i]; ++i1) {
            const auto start = std::chrono::high_resolution_clock::now();

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

            if (primitive.indicesAccessor.has_value()) {
                auto& accessor = asset.accessors[primitive.indicesAccessor.value()];

                uint32_t idx = 0;
                fastgltf::iterateAccessor<std::uint32_t>(asset, accessor, [&](const std::uint32_t index) {
                    this->indices[indicesCount[globalIndex] + idx] = index + baseVertices[globalIndex];
                    idx++;
                });
            }

            const auto end = std::chrono::high_resolution_clock::now();
            const std::chrono::duration<double> duration = end - start;

            totalGeometryTime += duration.count();

            this->meshes.emplace_back(std::move(tempMesh));

            if (primitive.materialIndex.has_value()) {
                auto& accessor = asset.materials[primitive.materialIndex.value()];
                if (accessor.pbrData.baseColorTexture.has_value()) {
                    const auto& texture = asset.textures[accessor.pbrData.baseColorTexture.value().textureIndex];

                    if (texture.basisuImageIndex.has_value() && lastIndex != texture.basisuImageIndex.value()) {
                        textureIndices.emplace_back(i, i1);
                        lastIndex = texture.basisuImageIndex.value();
                        globalIndices.emplace_back(globalIndex);

                        Texture modelTexture{};
                        modelTexture.indexOffset = baseVertices[globalIndex];
                        this->textures.emplace_back(modelTexture);
                    }
                }
            }

            globalIndex++;
        }
    }

    const auto start = std::chrono::high_resolution_clock::now();
    #pragma omp parallel for schedule(static) default(none) shared(textureIndices, asset, baseVertices, globalIndices)
    for (int i = 0; i < textureIndices.size(); ++i) {
        const auto& pair = textureIndices[i];
        const auto& primitive = asset.meshes[pair.first].primitives[pair.second];

        const auto& texture = asset.textures[asset.materials[primitive.materialIndex.value()].pbrData.baseColorTexture.value().textureIndex];

        auto& image = asset.images[texture.basisuImageIndex.value()];
        processImageData(image, asset, baseVertices[globalIndices[i]], i);
    }
    const auto end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> totalTextureTime = end - start;

    LOGGER.success("Loaded model textures in: ${}", totalTextureTime.count());
    LOGGER.success("Loaded model geometry in: ${}", totalGeometryTime);
}


void ParsedModel::processImageData(fastgltf::Image& image, fastgltf::Asset& asset, size_t index, size_t textureIndex) {
    auto LOGGER = Logger("processImageData()");

    const auto& data = image.data;
    if (const auto* view = std::get_if<fastgltf::sources::BufferView>(&data)) {
        const auto& bufferView = asset.bufferViews[view->bufferViewIndex];
        const auto& buffer = asset.buffers[bufferView.bufferIndex];

        if (const auto* array = std::get_if<fastgltf::sources::Array>(&buffer.data)) {
            basist::ktx2_transcoder transcoder;

            const auto* ktx2Ptr = reinterpret_cast<const uint8_t*>(array->bytes.data()) + bufferView.byteOffset;
            const auto ktx2Size = static_cast<uint32_t>(bufferView.byteLength);

            if (!transcoder.init(ktx2Ptr, ktx2Size)) {
                LOGGER.error("Bad ktx2 texture in your .glb model");
                return;
            }

            basist::ktx2_image_level_info info{};
            uint32_t level_index = 0;
            uint32_t layer_index = 0;
            uint32_t face_index = 0;
            transcoder.get_image_level_info(info, level_index, layer_index, face_index);

            constexpr auto targetBasisFormat = basist::transcoder_texture_format::cTFBC7_RGBA;

            uint32_t bytesPerBlock = basist::basis_get_bytes_per_block_or_pixel(targetBasisFormat);
            uint32_t totalSizeBytes = info.m_total_blocks * bytesPerBlock;

            std::vector<uint8_t> transcodedData(totalSizeBytes);

            const bool success = transcoder.transcode_image_level(
            level_index, layer_index, face_index,
            transcodedData.data(),
            info.m_total_blocks,
            targetBasisFormat
            );

            if (!success) {
                LOGGER.error("Failed to transcode image!");
                return;
            }

            Texture& texture = this->textures[textureIndex];

            texture.texWidth = info.m_width;
            texture.texHeight = info.m_height;
            texture.imageSize = transcodedData.size();
            texture.format = VK_FORMAT_BC7_UNORM_BLOCK;

            texture.pixels = new uint8_t[transcodedData.size()];
            std::memcpy(texture.pixels, transcodedData.data(), transcodedData.size());
        }
    }
}

// TODO GET THIS SHIT OUTTA HERE IN MODEL GROUPS
void ParsedModel::calcOcclusionSphere() {
    glm::vec3 modelCenter(0.0f);
    size_t totalVertices = 0;

    for (const auto& mesh : meshes) {
        for (const auto& v : mesh) {
            modelCenter += v.pos;
        }
        totalVertices += mesh.size();
    }
    if (totalVertices > 0) modelCenter /= static_cast<float>(totalVertices);

    float radius = 0.0f;
    for (const auto& mesh : meshes) {
        for (const auto& v : mesh) {
            radius = std::max(radius, glm::length(v.pos - modelCenter));
        }
    }
    sphere = glm::vec4(modelCenter, radius);
}

JPH::ShapeRefC ParsedModel::createJoltMesh() const {
    JPH::VertexList joltVertices;
    JPH::IndexedTriangleList joltTriangles;

    // 1. Считаем общее количество вершин для резервации памяти
    size_t totalVertices = 0;
    for (const auto& mesh : meshes) {
        totalVertices += mesh.size();
    }

    if (totalVertices == 0 || indices.empty()) {
        return nullptr;
    }

    joltVertices.reserve(totalVertices);
    joltTriangles.reserve(indices.size() / 3);

    // 2. Копируем вершины с ПРАВИЛЬНЫМ свопом осей для Z-up
    // Если в твоей модели (GLTF) Y - это вверх, а ты хочешь в Jolt Z - это вверх:
    // Модель(X, Y, Z) -> Jolt(X, -Z, Y) - это сохраняет ориентацию (Right-Handed)
    for (const auto& mesh : meshes) {
        for (const auto& v : mesh) {
            // Мапим: GLTF.Y в Jolt.Z (высота), GLTF.Z в Jolt.-Y (глубина)
            joltVertices.push_back(JPH::Float3(v.pos.x, v.pos.y, v.pos.z));
        }
    }

    // 3. Копируем индексы (Jolt ждет IndexedTriangle)
    // Важно: если при свопе осей сфера пролетает меш, значит winding order инвертировался.
    // settings.mAllowBackFaceCollision ниже решает эту проблему глобально.
    for (size_t i = 0; i < indices.size(); i += 3) {
        joltTriangles.push_back(JPH::IndexedTriangle(
            indices[i],
            indices[i + 1],
            indices[i + 2]
        ));
    }

    // 4. Настройка параметров меша
    JPH::MeshShapeSettings settings(joltVertices, joltTriangles);
    settings.mActiveEdgeCosThresholdAngle = -1.0f;

    // Генерируем форму
    JPH::Shape::ShapeResult result = settings.Create();

    if (result.HasError()) {
        // Если у тебя есть доступ к логгеру здесь:
        // LOGGER.error("Jolt Mesh Error: ${}", result.GetError().c_str());
        return nullptr;
    }

    return result.Get();
}