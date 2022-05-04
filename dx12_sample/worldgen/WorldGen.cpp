#include "WorldGen.h"

double Length(double x, double y)
{
    x -= 1.0;
    y -= 1.0;
    return sqrt(x * x + y * y);
}

constexpr double DLength(double x, double y)
{
    x -= 1.0;
    y -= 1.0;
    return x * x + y * y;
}

constexpr double GetAmplitude(int octaves, double persistance)
{
    double result = 0.0;

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

void WorldGen::GenerateHeightMap(int    octaves,
                                 double persistance,
                                 double frequency,
                                 double lacunarity,
                                 double offset,
                                 double multiplier)
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

            // calculate a length from the center of the map
            double length2 = Length(i, j) > 1.0 ? 1.0 : DLength(i, j);

            // adjust value according to the length from the center
            value *= (1 - length2 * length2);

            // adjust contast
            value = std::pow(value, 2.2);

            std::size_t cellX = (std::size_t)(i * _sideSize / 2);
            std::size_t cellY = (std::size_t)(j * _sideSize / 2);

            _heightMap[GetIndex(cellX, cellY, _sideSize)] = offset + value * multiplier;
        }
    }
}
