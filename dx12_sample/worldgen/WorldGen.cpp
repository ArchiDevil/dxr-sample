#include "WorldGen.h"

Noise& WorldGen::GetNoise()
{
    return _noise;
}

double Length(double x, double y)
{
    x -= 1.0;
    y -= 1.0;
    return sqrt(x * x + y * y);
}

constexpr std::size_t GetIndex(std::size_t x, std::size_t y, std::size_t sideSize)
{
    return x * sideSize + y;
}

constexpr double GetAmplitude(int octaves, double persistance)
{
    double result    = 0.0;

    double amplitude = 1.0;
    for (auto octave = 0; octave < octaves; ++octave)
    {
        result += amplitude;
        amplitude *= persistance;
    }

    return result;
}

WorldGen::WorldGen(std::size_t sideSize)
    : _sideSize(sideSize)
{
    _heightMap.resize(sideSize * sideSize);
}

void WorldGen::GenerateHeightMap(int octaves, double persistance, double frequency, double lacunarity)
{
    _noise.SetOctaves(octaves);
    _noise.SetPersistence(persistance);
    _noise.SetFrequency(frequency);
    _noise.SetLacunarity(lacunarity);

    const double amplitude = GetAmplitude(octaves, persistance);

    for (double i = 0.0; i < 2.0; i += 1.0 / _sideSize)
    {
        for (double j = 0.0; j < 2.0; j += 1.0 / _sideSize)
        {
            double value = _noise.SimplexNoise(i, j);

            value += amplitude;
            value /= amplitude * 2.0;

            double length2 = Length(i, j) > 1.0 ? 1.0 : Length(i, j) * Length(i, j);
            double curMult = 0.05 + 0.95 * length2;
            value *= (1 - curMult * curMult);

            std::size_t cellX = (std::size_t)(i * _sideSize / 2);
            std::size_t cellY = (std::size_t)(j * _sideSize / 2);

            _heightMap[GetIndex(cellX, cellY, _sideSize)] = 20 + value * 125.0;
        }
    }
}

// void WorldGen::GenerateHeightMap(int size, float density)
//{
//    int    mapSize = size;
//    double r       = 0.0;
//    //noise.SetOctaves(8);
//
//    for (double i = 0.0; i < 2.0; i += 1.0 / mapSize)
//    {
//        for (double j = 0.0; j < 2.0; j += 1.0 / mapSize)
//        {
//            double value = noise.SimplexNoise(i, j);
//
//            value += 1.0;
//            value /= 2.0;
//
//            double length2 = length(i, j) > 1.0 ? 1.0 : length(i, j) * length(i, j);
//            double curMult = 0.05 + 0.95 * length2;
//            value *= (1 - curMult * curMult);
//
//            int cellX = (int)(i * mapSize / 2);
//            int cellY = (int)(j * mapSize / 2);
//
//            _heightMap[cellX][cellY] = value * density;
//        }
//    }
//}

void WorldGen::GenerateHeightMap2(int size, float density)
{
    float scale       = 500.f;
    float offset_x    = 5.022f;
    float offset_y    = 4.22f;
    float offset_z    = 0.05f;
    float lacunarity  = 1.5f;
    float persistance = 0.5f;

    const SimplexNoise simplex(1.1f / scale, 1.0f, lacunarity, persistance);
    const int octaves = static_cast<int>(5 + std::log(scale));  // Estimate number of octaves needed for the current scale

    for (int row = 0; row <= size; ++row)
    {
        const float y = static_cast<float>(row - size / 2 + offset_y * scale);
        for (int col = 0; col <= size; ++col)
        {
            const float x = static_cast<float>(col - size / 2 + offset_x * scale);

            const float noise = simplex.fractal(octaves, x, y) + offset_z;

            _heightMap[GetIndex(col, row, _sideSize)] = 20 + noise * density;
        }
    }
}

uint8_t WorldGen::GetHeight(std::size_t x, std::size_t y)
{
    return _heightMap[GetIndex(x, y, _sideSize)];
}

std::size_t WorldGen::GetSideSize() const
{
    return _sideSize;
}
