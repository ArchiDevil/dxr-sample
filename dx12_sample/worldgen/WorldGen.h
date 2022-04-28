#pragma once

#include "Noise.h"
#include "SimplexNoise.h"

#include <cmath>
#include <fstream>
#include <string>

class WorldGen
{
public:
    WorldGen() = default;

    void GenerateHeightMap(int size, float density = 100000.0);
    void GenerateHeightMap2(int size, float density = 100000.0);
    int  GetHeight(int ChunkX, int ChunkY);

    Noise& GetNoise();

private:
    Noise noise;
    SimplexNoise noise2;


    static const int _worldSize = 1024;
    int              _heightMap[_worldSize][_worldSize] = {};
};
