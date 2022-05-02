#pragma once

#include "Noise.h"
#include "SimplexNoise.h"

#include <cstdint>
#include <vector>

constexpr std::size_t GetIndex(std::size_t x, std::size_t y, std::size_t sideSize)
{
    return x * sideSize + y;
}

class WorldGen
{
public:
    WorldGen(std::size_t sideSize);

    void GenerateHeightMap(int    octaves,
                           double persistance,
                           double frequency,
                           double lacunarity,
                           double offset     = 20.0,
                           double multiplier = 125.0);
    void GenerateHeightMap2();

    uint8_t GetHeight(int x, int y) const
    {
        if (x >= _sideSize || x < 0 || y >= _sideSize || y < 0)
            return 0;

        return _heightMap.at(GetIndex(x, y, _sideSize));
    }

    std::size_t GetSideSize() const
    {
        return _sideSize;
    }

    Noise& GetNoise()
    {
        return _noise;
    }

private:
    Noise                _noise;
    std::size_t          _sideSize;
    std::vector<uint8_t> _heightMap;
};
