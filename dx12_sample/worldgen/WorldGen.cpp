#include "WorldGen.h"

double length(double x, double y)
{
    return sqrt(pow(x - 1.0, 2.0) + pow(y - 1.0, 2.0));
}

void WorldGen::GenerateHeightMap(int size, float density)
{
    int    mapSize = size;
    double r       = 0.0;
    noise.SetOctaves(8);
    //noise.SetFrequency(1.0 + r / 10.0);

    for (double i = 0.0; i < 2.0; i += 1.0 / mapSize)
    {
        for (double j = 0.0; j < 2.0; j += 1.0 / mapSize)
        {
            double value = noise.SimplexNoise(i, j);

            value += 1.0;
            value /= 2.0;

            double length2 = length(i, j) > 1.0 ? 1.0 : length(i, j) * length(i, j);
            double curMult = 0.05 + 0.95 * length2;
            value *= (1 - curMult * curMult);

            int cellX = (int)(i * mapSize / 2);
            int cellY = (int)(j * mapSize / 2);

            _heightMap[cellX][cellY] = value * density;
        }
    }
}

void WorldGen::GenerateHeightMap2(int size, float density)
{
    float scale       = 279.f;
    float offset_x    = 5.022f;
    float offset_y    = 4.22f;
    float offset_z    = 0.05f;
    float lacunarity  = 1.5f;
    float persistance = 0.5f;

    const SimplexNoise simplex(1.1f / scale, 15.5f, lacunarity, persistance);  // Amplitude of 0.5 for the 1st octave : sum ~1.0f
    const int octaves = static_cast<int>(5 + std::log(scale));  // Estimate number of octaves needed for the current scale

    for (int row = 0; row <= size; ++row)
    {
        const float y = static_cast<float>(row - size / 2 + offset_y * scale);
        for (int col = 0; col <= size; ++col)
        {
            const float x = static_cast<float>(col - size / 2 + offset_x * scale);

            const float   noise = simplex.fractal(octaves, x, y) + offset_z;
            _heightMap[col][row] = noise * density;
        }
    }
}

int WorldGen::GetHeight(int ChunkX, int ChunkY)
{
    const int div = 1; //2
    if (ChunkX < _worldSize / div && ChunkX > -_worldSize / div && ChunkY < _worldSize / div && ChunkY > -_worldSize / div)
    {
        const int offset = 0;  //_worldSize / 2
        return _heightMap[ChunkX + offset][ChunkY + offset];
    }
    else
    {
        return 0;
    }
}

Noise& WorldGen::GetNoise()
{
    return noise;
}
