#include "TerrainManager.h"

TerrainManager::TerrainManager(const WorldGen&                   worldGenerator,
                               const std::map<uint8_t, XMUINT3>& colorsLut,
                               std::size_t                       chunkSize /* = 128*/)
    : _worldGenerator(worldGenerator)
    , _chunkSize(chunkSize)
    , _colorsLut(colorsLut)
{
    GenerateChunks();
}

void TerrainManager::GenerateChunks()
{
    for (int x = 0; x < _worldGenerator.GetSideSize(); x += _chunkSize)
    {
        for (int y = 0; y < _worldGenerator.GetSideSize(); y += _chunkSize)
        {
            _chunks.emplace_back(GenerateChunk(x, y));
        }
    }
}

TerrainChunk TerrainManager::GenerateChunk(int startX, int startY)
{
    const std::size_t mapSize     = _worldGenerator.GetSideSize();
    const float       blockWidth  = 1.0f;
    const float       islandWidth = blockWidth * mapSize;

    std::vector<GeometryVertex> vertices;
    vertices.reserve(_chunkSize * _chunkSize * 8);

    std::vector<uint32_t> indices;
    indices.reserve(vertices.size() * 4);

    for (int y = startY; y < startY + _chunkSize; y++)
    {
        for (int x = startX; x < startX + _chunkSize; x++)
        {
            const float nz = _worldGenerator.GetHeight(x, y);
            const float nx = -(_chunkSize / 2.0f) + x - startX;
            const float ny = -(_chunkSize / 2.0f) + y - startY;
            GenerateCube({nx, ny, nz}, vertices, indices);
        }
    }

    return TerrainChunk{startX, startY, std::move(vertices), std::move(indices)};
}

void TerrainManager::GenerateCube(XMFLOAT3 topPoint, std::vector<GeometryVertex>& vertices, std::vector<uint32_t>& indices)
{
    uint32_t beg_vertex = vertices.size();

    const float offset = 0.5f;

    const float x = topPoint.x + offset;
    const float y = topPoint.y + offset;
    const float z = topPoint.z + offset;

    const float bottom = 0.0f;

    auto     colorX = _colorsLut.lower_bound(z)->second;
    XMFLOAT3 color  = {colorX.x / 255.0f, colorX.y / 255.0f, colorX.z / 255.0f};

    const std::array vertices1 = {
        // back face +Z
        GeometryVertex{{x + 0.5f, y + 0.5f, z + 0.5f}, {0.0f, 0.0f, 1.0f}, color},
        GeometryVertex{{x + 0.5f, y + -0.5f, z + 0.5f}, {0.0f, 0.0f, 1.0f}, color},
        GeometryVertex{{x + -0.5f, y + 0.5f, z + 0.5f}, {0.0f, 0.0f, 1.0f}, color},
        GeometryVertex{{x + -0.5f, y + -0.5f, z + 0.5f}, {0.0f, 0.0f, 1.0f}, color},

        // front face -Z
        GeometryVertex{{x + 0.5f, y + 0.5f, bottom}, {0.0f, 0.0f, -1.0f}, color},
        GeometryVertex{{x + 0.5f, y + -0.5f, bottom}, {0.0f, 0.0f, -1.0f}, color},
        GeometryVertex{{x + -0.5f, y + 0.5f, bottom}, {0.0f, 0.0f, -1.0f}, color},
        GeometryVertex{{x + -0.5f, y + -0.5f, bottom}, {0.0f, 0.0f, -1.0f}, color},

        // bottom face -Y
        GeometryVertex{{x + -0.5f, y + -0.5f, z + 0.5f}, {0.0f, -1.0f, 0.0f}, color},
        GeometryVertex{{x + 0.5f, y + -0.5f, z + 0.5f}, {0.0f, -1.0f, 0.0f}, color},
        GeometryVertex{{x + 0.5f, y + -0.5f, bottom}, {0.0f, -1.0f, 0.0f}, color},
        GeometryVertex{{x + -0.5f, y + -0.5f, bottom}, {0.0f, -1.0f, 0.0f}, color},

        // top face +Y
        GeometryVertex{{x + -0.5f, y + 0.5f, z + 0.5f}, {0.0f, 1.0f, 0.0f}, color},
        GeometryVertex{{x + 0.5f, y + 0.5f, z + 0.5f}, {0.0f, 1.0f, 0.0f}, color},
        GeometryVertex{{x + 0.5f, y + 0.5f, bottom}, {0.0f, 1.0f, 0.0f}, color},
        GeometryVertex{{x + -0.5f, y + 0.5f, bottom}, {0.0f, 1.0f, 0.0f}, color},

        // left face -X
        GeometryVertex{{x + -0.5f, y + 0.5f, z + 0.5f}, {-1.0f, 0.0f, 0.0f}, color},
        GeometryVertex{{x + -0.5f, y + -0.5f, z + 0.5f}, {-1.0f, 0.0f, 0.0f}, color},
        GeometryVertex{{x + -0.5f, y + -0.5f, bottom}, {-1.0f, 0.0f, 0.0f}, color},
        GeometryVertex{{x + -0.5f, y + 0.5f, bottom}, {-1.0f, 0.0f, 0.0f}, color},

        // right face +X
        GeometryVertex{{x + 0.5f, y + 0.5f, z + 0.5f}, {1.0f, 0.0f, 0.0f}, color},
        GeometryVertex{{x + 0.5f, y + -0.5f, z + 0.5f}, {1.0f, 0.0f, 0.0f}, color},
        GeometryVertex{{x + 0.5f, y + -0.5f, bottom}, {1.0f, 0.0f, 0.0f}, color},
        GeometryVertex{{x + 0.5f, y + 0.5f, bottom}, {1.0f, 0.0f, 0.0f}, color},
    };
    vertices.insert(vertices.end(), vertices1.begin(), vertices1.end());

    const std::array indices2 = {
        // back
        beg_vertex + 0, beg_vertex + 2, beg_vertex + 1, beg_vertex + 1, beg_vertex + 2, beg_vertex + 3,
        // front +4
        beg_vertex + 4, beg_vertex + 5, beg_vertex + 7, beg_vertex + 4, beg_vertex + 7, beg_vertex + 6,
        // left +8
        beg_vertex + 9, beg_vertex + 8, beg_vertex + 10, beg_vertex + 10, beg_vertex + 8, beg_vertex + 11,
        // right +12
        beg_vertex + 13, beg_vertex + 14, beg_vertex + 12, beg_vertex + 14, beg_vertex + 15, beg_vertex + 12,
        // front +16
        beg_vertex + 17, beg_vertex + 16, beg_vertex + 19, beg_vertex + 18, beg_vertex + 17, beg_vertex + 19,
        // back +20
        beg_vertex + 21, beg_vertex + 23, beg_vertex + 20, beg_vertex + 22, beg_vertex + 23, beg_vertex + 21};
    indices.insert(indices.end(), indices2.begin(), indices2.end());
}
