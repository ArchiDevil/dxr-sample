#include "WorldGen.h"

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

uint8_t WorldGen::GetHeight(std::size_t x, std::size_t y)
{
    return _heightMap[GetIndex(x, y, _sideSize)];
}
