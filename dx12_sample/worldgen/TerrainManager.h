#pragma once

#include "WorldGen.h"

#include <shaders/Common.h>
#include <utils/GeometryTree.h>

#include <cstdint>

struct TerrainChunk
{
    int absX, absY;

    std::vector<GeometryVertex> landVertices;
    std::vector<uint32_t>       landIndices;

    std::vector<GeometryVertex> waterVertices;
    std::vector<uint32_t>       waterIndices;
};

class TerrainManager
{
public:
    TerrainManager(const WorldGen&                   worldGenerator,
                   const std::map<uint8_t, XMUINT3>& colorsLut,
                   std::size_t                       chunkSize  = 128,
                   std::size_t                       waterLevel = 55);

    const std::vector<TerrainChunk>& GetChunks() const
    {
        return _chunks;
    }

private:
    void         GenerateChunks();
    TerrainChunk GenerateChunk(int startX, int startY);

    void GenerateLandCol(int                          x,
                         int                          y,
                         uint32_t&                    currentIdx,
                         std::vector<GeometryVertex>& vertices,
                         std::vector<uint32_t>&       indices,
                         int                          startX,
                         int                          startY);

    void GenerateWaterQuad(int                          x,
                           int                          y,
                           uint32_t&                    currentIdx,
                           std::vector<GeometryVertex>& vertices,
                           std::vector<uint32_t>&       indices,
                           int                          startX,
                           int                          startY);

    void GenerateWaterQuad(const TreeNode&              nodeTree,
                           std::vector<GeometryVertex>& vertices,
                           std::vector<uint32_t>&       indices,
                           uint32_t&                    currentIdx);

    const std::size_t                 _chunkSize;
    const std::size_t                 _waterLevel;
    const WorldGen&                   _worldGenerator;
    const std::map<uint8_t, XMUINT3>& _colorsLut;
    std::vector<TerrainChunk>         _chunks;
};
