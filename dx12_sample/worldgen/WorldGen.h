#pragma once

#include "Noise.h"

#include <cmath>
#include <fstream>
#include <string>

class WorldGen
{
public:
    WorldGen(std::size_t sideSize);
    void    GenerateHeightMap(int octaves, double persistance, double frequency, double lacunarity);
    uint8_t GetHeight(std::size_t x, std::size_t y);

private:
    Noise                _noise;
    std::size_t          _sideSize;
    std::vector<uint8_t> _heightMap;
};
