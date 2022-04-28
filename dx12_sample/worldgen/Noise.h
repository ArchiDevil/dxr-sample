#pragma once

double dot(int* g, double x, double y);
double dot(int* g, double x, double y, double z);

class Noise
{
public:
    Noise();

    double SimplexNoise(double x, double y, double z);
    double SimplexNoise(double x, double y);

    void SetFrequency(double freq);
    void SetPersistence(double pers);
    void SetLacunarity(double lac);
    void SetSeed(int seed);
    void SetOctaves(int oct);

private:
    double GetNoise(double xin, double yin);
    double GetNoise(double xin, double yin, double zin);

    int _grad3[12][3];
    int _perm[512];

    int    _octaves;
    double _frequency;
    double _persistence;
    double _lacunarity;
};
