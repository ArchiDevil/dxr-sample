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

    void        GenerateHeightMap(int    octaves,
                                  double persistance,
                                  double frequency,
                                  double lacunarity,
                                  double offset     = 20.0,
                                  double multiplier = 125.0);
    void        GenerateHeightMap2();
    uint8_t     GetHeight(std::size_t x, std::size_t y) const;
    std::size_t GetSideSize() const;

    Noise& GetNoise();

private:
    Noise                _noise;
    std::size_t          _sideSize;
    std::vector<uint8_t> _heightMap;
};
