#pragma once

#include "Noise.h"
#include "SimplexNoise.h"

#include <cmath>
#include <fstream>
#include <string>

class WorldGen
{
public:
    WorldGen(std::size_t sideSize);

    void        GenerateHeightMap(int octaves, double persistance, double frequency, double lacunarity);
    void        GenerateHeightMap2(int size, float density = 125.0f);
    uint8_t     GetHeight(std::size_t x, std::size_t y);
    std::size_t GetSideSize() const;

    Noise& GetNoise();

private:
    Noise       _noise;
    std::size_t _sideSize;

    SimplexNoise _noise2;

    static const int     _worldSize = 1024;
    std::vector<uint8_t> _heightMap;
};
