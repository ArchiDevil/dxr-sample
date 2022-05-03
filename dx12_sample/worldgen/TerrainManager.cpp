#include "TerrainManager.h"

#include <iostream>

TerrainManager::TerrainManager(const WorldGen&                   worldGenerator,
                               const std::map<uint8_t, XMUINT3>& colorsLut,
                               std::size_t                       chunkSize /* = 128*/,
                               std::size_t                       waterLevel /*= 55*/)
    : _worldGenerator(worldGenerator)
    , _chunkSize(chunkSize)
    , _colorsLut(colorsLut)
    , _waterLevel(waterLevel)
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

    std::size_t totalIndicesCount = 0;
    for (auto& chunk : _chunks)
    {
        totalIndicesCount += chunk.landIndices.size();
        totalIndicesCount += chunk.waterIndices.size();
    }

    std::cout << "Total indices count: " << totalIndicesCount << ", triangles = " << totalIndicesCount / 3 << std::endl;
}

TerrainChunk TerrainManager::GenerateChunk(int startX, int startY)
{
    const std::size_t mapSize     = _worldGenerator.GetSideSize();
    const float       blockWidth  = 1.0f;
    const float       islandWidth = blockWidth * mapSize;

    TerrainChunk output{(int)(startX + _chunkSize / 2), (int)(startY + _chunkSize / 2)};

    output.landVertices.reserve(_chunkSize * _chunkSize * 8);
    output.landIndices.reserve(output.landVertices.capacity() * 4);

    uint32_t currentLandIdx  = 0;
    uint32_t currentWaterIdx = 0;

    GeometryTree waterTree{(float)_chunkSize, 1.0f};

    for (int x = startX; x < startX + _chunkSize; x++)
    {
        for (int y = startY; y < startY + _chunkSize; y++)
        {
            GenerateLandCol(x, y, currentLandIdx, output.landVertices, output.landIndices, startX, startY);

            // old way to generate geometry
            // GenerateWaterQuad(x, y, currentWaterIdx, output.waterVertices, output.waterIndices, startX, startY);

            {
                const float height     = _waterLevel;
                const float landHeight = _worldGenerator.GetHeight(x, y);
                if (landHeight >= height)
                    waterTree.AddPoint(x - startX + 0.5f, y - startY + 0.5f);
            }
        }
    }

    GenerateWaterQuad(waterTree.GetRoot(), output.waterVertices, output.waterIndices, currentWaterIdx);

    return output;
}

void TerrainManager::GenerateLandCol(int                          x,
                                     int                          y,
                                     uint32_t&                    currentIdx,
                                     std::vector<GeometryVertex>& vertices,
                                     std::vector<uint32_t>&       indices,
                                     int                          startX,
                                     int                          startY)
{
    const float    height = _worldGenerator.GetHeight(x, y);
    const float    xpos   = x - startX;
    const float    ypos   = y - startY;
    const auto     colorX = _colorsLut.lower_bound(height)->second;
    const XMFLOAT3 color  = {colorX.x / 255.0f, colorX.y / 255.0f, colorX.z / 255.0f};

    // generate +Z edge
    {
        std::array topVertices = {
            GeometryVertex{{xpos + 0.5f, ypos + 0.5f, height}, {0.0f, 0.0f, 1.0f}, color},
            GeometryVertex{{xpos + 0.5f, ypos - 0.5f, height}, {0.0f, 0.0f, 1.0f}, color},
            GeometryVertex{{xpos - 0.5f, ypos + 0.5f, height}, {0.0f, 0.0f, 1.0f}, color},
            GeometryVertex{{xpos - 0.5f, ypos - 0.5f, height}, {0.0f, 0.0f, 1.0f}, color},
        };

        vertices.insert(vertices.end(), topVertices.cbegin(), topVertices.cend());
        indices.insert(indices.end(),
                       {currentIdx + 0, currentIdx + 2, currentIdx + 1, currentIdx + 1, currentIdx + 2, currentIdx + 3});
        currentIdx += 4;
    }

    // generate +X edge
    float heightDiff = height - _worldGenerator.GetHeight(x + 1, y);
    if (heightDiff > 0.0)  // current block is higher
    {
        XMFLOAT3   normal        = {1.0f, 0.0f, 0.0f};
        std::array rightVertices = {
            GeometryVertex{{xpos + 0.5f, ypos - 0.5f, height}, normal, color},
            GeometryVertex{{xpos + 0.5f, ypos - 0.5f, height - heightDiff}, normal, color},
            GeometryVertex{{xpos + 0.5f, ypos + 0.5f, height - heightDiff}, normal, color},
            GeometryVertex{{xpos + 0.5f, ypos + 0.5f, height}, normal, color},
        };
        vertices.insert(vertices.end(), rightVertices.cbegin(), rightVertices.cend());
        indices.insert(indices.end(),
                       {currentIdx + 0, currentIdx + 1, currentIdx + 3, currentIdx + 1, currentIdx + 2, currentIdx + 3});
        currentIdx += 4;
    }

    // generate -X edge
    heightDiff = height - _worldGenerator.GetHeight(x - 1, y);
    if (heightDiff > 0.0)  // current block is higher
    {
        XMFLOAT3   normal       = {-1.0f, 0.0f, 0.0f};
        std::array leftVertices = {
            GeometryVertex{{xpos - 0.5f, ypos - 0.5f, height}, normal, color},
            GeometryVertex{{xpos - 0.5f, ypos - 0.5f, height - heightDiff}, normal, color},
            GeometryVertex{{xpos - 0.5f, ypos + 0.5f, height - heightDiff}, normal, color},
            GeometryVertex{{xpos - 0.5f, ypos + 0.5f, height}, normal, color},
        };
        vertices.insert(vertices.end(), leftVertices.cbegin(), leftVertices.cend());
        indices.insert(indices.end(),
                       {currentIdx + 0, currentIdx + 3, currentIdx + 1, currentIdx + 1, currentIdx + 3, currentIdx + 2});
        currentIdx += 4;
    }

    // generate +Y edge
    heightDiff = height - _worldGenerator.GetHeight(x, y + 1);
    if (heightDiff > 0.0)  // current block is higher
    {
        XMFLOAT3   normal        = {0.0f, 1.0f, 0.0f};
        std::array frontVertices = {
            GeometryVertex{{xpos + 0.5f, ypos + 0.5f, height}, normal, color},
            GeometryVertex{{xpos + 0.5f, ypos + 0.5f, height - heightDiff}, normal, color},
            GeometryVertex{{xpos - 0.5f, ypos + 0.5f, height - heightDiff}, normal, color},
            GeometryVertex{{xpos - 0.5f, ypos + 0.5f, height}, normal, color},
        };
        vertices.insert(vertices.end(), frontVertices.cbegin(), frontVertices.cend());
        indices.insert(indices.end(),
                       {currentIdx + 0, currentIdx + 1, currentIdx + 2, currentIdx + 2, currentIdx + 3, currentIdx + 0});
        currentIdx += 4;
    }

    // generate -Y edge
    heightDiff = height - _worldGenerator.GetHeight(x, y - 1);
    if (heightDiff > 0.0)  // current block is higher
    {
        XMFLOAT3   normal       = {0.0f, -1.0f, 0.0f};
        std::array backVertices = {
            GeometryVertex{{xpos + 0.5f, ypos - 0.5f, height}, normal, color},
            GeometryVertex{{xpos + 0.5f, ypos - 0.5f, height - heightDiff}, normal, color},
            GeometryVertex{{xpos - 0.5f, ypos - 0.5f, height - heightDiff}, normal, color},
            GeometryVertex{{xpos - 0.5f, ypos - 0.5f, height}, normal, color},
        };
        vertices.insert(vertices.end(), backVertices.cbegin(), backVertices.cend());
        indices.insert(indices.end(),
                       {currentIdx + 1, currentIdx + 0, currentIdx + 2, currentIdx + 2, currentIdx + 0, currentIdx + 3});
        currentIdx += 4;
    }
}

void TerrainManager::GenerateWaterQuad(int                          x,
                                       int                          y,
                                       uint32_t&                    currentIdx,
                                       std::vector<GeometryVertex>& vertices,
                                       std::vector<uint32_t>&       indices,
                                       int                          startX,
                                       int                          startY)
{
    const float    height     = _waterLevel;
    const float    landHeight = _worldGenerator.GetHeight(x, y);
    const float    xpos       = x - startX;
    const float    ypos       = y - startY;
    const XMFLOAT3 color      = {0.0f, 0.0f, 0.0f};  // is not used
    const XMFLOAT3 normal     = {0.0f, 0.0f, 1.0f};

    if (landHeight > height)
        return;

    // generate +Z edge
    {
        std::array topVertices = {
            GeometryVertex{{xpos + 0.5f, ypos + 0.5f, height - .5f}, normal, color},
            GeometryVertex{{xpos + 0.5f, ypos - 0.5f, height - .5f}, normal, color},
            GeometryVertex{{xpos - 0.5f, ypos + 0.5f, height - .5f}, normal, color},
            GeometryVertex{{xpos - 0.5f, ypos - 0.5f, height - .5f}, normal, color},
        };

        vertices.insert(vertices.end(), topVertices.cbegin(), topVertices.cend());
        indices.insert(indices.end(),
                       {currentIdx + 0, currentIdx + 2, currentIdx + 1, currentIdx + 1, currentIdx + 2, currentIdx + 3});
        currentIdx += 4;
    }
}

void TerrainManager::GenerateWaterQuad(const TreeNode&              nodeTree,
                                       std::vector<GeometryVertex>& vertices,
                                       std::vector<uint32_t>&       indices,
                                       uint32_t&                    currentIdx)
{
    const bool  isLeaf = nodeTree.children.empty();
    const float height = _waterLevel;

    if (isLeaf && !nodeTree.hasObject)
    {
        const XMFLOAT3 color  = {0.0f, 0.0f, 0.0f};  // is not used
        const XMFLOAT3 normal = {0.0f, 0.0f, 1.0f};

        // generate +Z edge
        {
            std::array topVertices = {
                GeometryVertex{
                    {nodeTree.xPos + nodeTree.size - .5f, nodeTree.yPos + nodeTree.size - .5f, height - .5f}, normal, color},
                GeometryVertex{{nodeTree.xPos + nodeTree.size - .5f, nodeTree.yPos - .5f, height - .5f}, normal, color},
                GeometryVertex{{nodeTree.xPos - .5f, nodeTree.yPos + nodeTree.size - .5f, height - .5f}, normal, color},
                GeometryVertex{{nodeTree.xPos - .5f, nodeTree.yPos - .5f, height - .5f}, normal, color},
            };

            vertices.insert(vertices.end(), topVertices.cbegin(), topVertices.cend());
            indices.insert(indices.end(), {currentIdx + 0, currentIdx + 2, currentIdx + 1, currentIdx + 1,
                                           currentIdx + 2, currentIdx + 3});
            currentIdx += 4;
        }
    }
    else
    {
        for (const auto& child : nodeTree.children)
        {
            GenerateWaterQuad(child, vertices, indices, currentIdx);
        }
    }
}
