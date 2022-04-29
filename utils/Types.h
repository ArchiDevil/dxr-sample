#pragma once

enum optTypes
{
};

enum class ShaderType
{
    Vertex,
    Pixel,
    Geometry,
    Hull,
    Domain,
    Compute
};

struct screenQuadVertex
{
    float position[2];
    float uv[2];
};

struct CommandLineOptions
{
};
