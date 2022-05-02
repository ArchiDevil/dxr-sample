#pragma once

#include "WorldGen.h"

#include <shaders/Common.h>

#include <cstdint>

struct TerrainChunk
{
    int absX, absY;

    std::vector<GeometryVertex> vertices;
    std::vector<uint32_t>       indices;
};

class TerrainManager
{
public:
    TerrainManager(const WorldGen& worldGenerator, const std::map<uint8_t, XMUINT3>& colorsLut, std::size_t chunkSize = 128);

    const std::vector<TerrainChunk>& GetChunks() const
    {
        return _chunks;
    }

private:
    void         GenerateChunks();
    TerrainChunk GenerateChunk(int startX, int startY);

    const std::size_t                 _chunkSize;
    const WorldGen&                   _worldGenerator;
    const std::map<uint8_t, XMUINT3>& _colorsLut;
    std::vector<TerrainChunk>         _chunks;
};
