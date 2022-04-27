#include "WorldGen.h"

double length(double x, double y)
{
    return sqrt(pow(x - 1.0, 2.0) + pow(y - 1.0, 2.0));
}

void WorldGen::GenerateHeightMap(int size)
{
    int    mapSize = size;
    double r       = 0.0;
    noise.SetOctaves(8);
    noise.SetFrequency(1.0 + r / 10.0);

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

            _heightMap[cellX][cellY] = value * 100.0;
        }
    }
}

int WorldGen::GetHeight(int ChunkX, int ChunkY)
{
    if (ChunkX < _worldSize / 2 && ChunkX > -_worldSize / 2 && ChunkY < _worldSize / 2 && ChunkY > -_worldSize / 2)
    {
        const int offset = 0; //_worldSize / 2
        return _heightMap[ChunkX + offset][ChunkY + offset];
    }
    else
    {
        return 0;
    }
}
