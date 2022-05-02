#include "stdafx.h"

#include "MeshManager.h"

#include "Types.h"

static const XMFLOAT3 defaultColor = {0.5f, 0.5f, 0.5f};

static const std::vector<GeometryVertex> cubeVertices =
{
    // back face +Z
    {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, defaultColor},
    {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, defaultColor},
    {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, defaultColor},
    {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, defaultColor},

    // front face -Z
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, defaultColor},
    {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, defaultColor},
    {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, defaultColor},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, defaultColor},

    // bottom face -Y
    {{-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, defaultColor},
    {{0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, defaultColor},
    {{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, defaultColor},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, defaultColor},

    // top face +Y
    {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, defaultColor},
    {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, defaultColor},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, defaultColor},
    {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, defaultColor},

    // left face -X
    {{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, defaultColor},
    {{-0.5f, -0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, defaultColor},
    {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, defaultColor},
    {{-0.5f, 0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, defaultColor},

    // right face +X
    {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, defaultColor},
    {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, defaultColor},
    {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, defaultColor},
    {{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, defaultColor},

    // back faces

    // back face +Z
    {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, -1.0f}, defaultColor},
    {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, -1.0f}, defaultColor},
    {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, -1.0f}, defaultColor},
    {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, -1.0f}, defaultColor},

    // front face -Z
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, defaultColor},
    {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, defaultColor},
    {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, defaultColor},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, defaultColor},

    // bottom face -Y
    {{-0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, defaultColor},
    {{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, defaultColor},
    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, defaultColor},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, defaultColor},

    // top face +Y
    {{-0.5f, 0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, defaultColor},
    {{0.5f, 0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, defaultColor},
    {{0.5f, 0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, defaultColor},
    {{-0.5f, 0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, defaultColor},

    // left face -X
    {{-0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, defaultColor},
    {{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, defaultColor},
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, defaultColor},
    {{-0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, defaultColor},

    // right face +X
    {{0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, defaultColor},
    {{0.5f, -0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, defaultColor},
    {{0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, defaultColor},
    {{0.5f, 0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, defaultColor},
};

MeshManager::MeshManager(ComPtr<ID3D12Device5> device)
    : _device(device)
{
}

std::shared_ptr<MeshObject> MeshManager::CreateCube(std::function<void(CommandList&)> cmdListExecutor)
{
    if (_cube)
        return _cube;

    static const std::vector<uint32_t> indices =
    {
        //back
        0, 2, 1,
        1, 2, 3,
        //front +4
        4, 5, 7,
        4, 7, 6,
        //left +8
        9, 8, 10,
        10, 8, 11,
        //right +12
        13, 14, 12,
        14, 15, 12,
        //front +16
        17, 16, 19,
        18, 17, 19,
        //back +20
        21, 23, 20,
        22, 23, 21
    };

    _cube = std::make_shared<MeshObject>(
        std::vector<uint8_t>{(uint8_t*)cubeVertices.data(), (uint8_t*)(cubeVertices.data() + cubeVertices.size())},
        sizeof(GeometryVertex),
        indices,
        _device,
        true,
        cmdListExecutor);

    return _cube;
}

std::shared_ptr<MeshObject> MeshManager::CreateEmptyCube(std::function<void(CommandList&)> cmdListExecutor)
{
    if (_emptyCube)
        return _emptyCube;

    static const std::vector<uint32_t> indices =
    {
        //back
        0, 2, 1,
        1, 2, 3,
        24 + 0, 24 + 1, 24 + 2,
        24 + 1, 24 + 3, 24 + 2,
        //front
        4, 5, 7,
        4, 7, 6,
        24 + 4, 24 + 7, 24 + 5,
        24 + 4, 24 + 6, 24 + 7,
        // right
        17, 16, 19,
        18, 17, 19,
        24 + 16, 24 + 17, 24 + 19,
        24 + 17, 24 + 18, 24 + 19,
        // left
        21, 23, 20,
        22, 23, 21,
        24 + 23, 24 + 21, 24 + 20,
        24 + 23, 24 + 22, 24 + 21
    };

    _emptyCube = std::make_shared<MeshObject>(
        std::vector<uint8_t>{(uint8_t*)cubeVertices.data(), (uint8_t*)(cubeVertices.data() + cubeVertices.size())},
        sizeof(GeometryVertex), indices, _device, true, cmdListExecutor);

    return _emptyCube;
}

std::shared_ptr<MeshObject> MeshManager::CreatePlane(std::function<void(CommandList&)> cmdListExecutor)
{
    if (_plane)
        return _plane;

    static const std::vector<GeometryVertex> vertices =
    {
        {{-0.5f, 0.0f, 0.5f}, {0.0f, 1.0f, 0.0f}, defaultColor},
        {{0.5f, 0.0f, 0.5f}, {0.0f, 1.0f, 0.0f}, defaultColor},
        {{0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, defaultColor},
        {{-0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, defaultColor},
    };

    static const std::vector<uint32_t> indices =
    {
        0, 1, 2,
        0, 2, 3
    };

    _plane = std::make_shared<MeshObject>(
        std::vector<uint8_t>{(uint8_t*)vertices.data(), (uint8_t*)(vertices.data() + vertices.size())},
        sizeof(GeometryVertex), indices, _device, true, cmdListExecutor);

    return _plane;
}

std::shared_ptr<MeshObject> MeshManager::CreateAxes(std::function<void(CommandList&)> cmdListExecutor)
{
    if (_axes)
        return _axes;

    const float                              axisW = 0.04f;
    const float                              axisL = 4.0f;
    float3                                   color = { 0.2f, 0.2f, 0.2f };
    static const std::vector<GeometryVertex> vertices = {
        //x/z
        {{0.0f, axisW, 0.0f}, {0.0f, 1.0f, 0.0f}, color},
        {{0.0f, -axisW, 0.0f}, {0.0f, 1.0f, 0.0f}, color},
        {{axisL, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, color},

        //y
        {{axisW, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, color},
        {{-axisW, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, color},
        {{0.0f, axisL, 0.0f}, {0.0f, 1.0f, 0.0f}, color},

        //z
        {{0.0f, 0.0f, axisL}, {0.0f, 1.0f, 0.0f}, color}
    };

    static const std::vector<uint32_t> indices = {// x - axis
                                                  0, 1, 2, 0, 2, 1,
                                                  // y
                                                  3, 4, 5, 5, 4, 3,
                                                  // z
                                                  0, 6, 1, 1, 6, 0,
    };

    _axes = std::make_shared<MeshObject>(
        std::vector<uint8_t>{(uint8_t*)vertices.data(), (uint8_t*)(vertices.data() + vertices.size())},
        sizeof(GeometryVertex), indices, _device, true, cmdListExecutor);

    return _axes;
}

std::shared_ptr<MeshObject> MeshManager::CreateScreenQuad()
{
    if (_screenQuad)
        return _screenQuad;

    static std::vector<screenQuadVertex> sqVertices =
    {
        {{-1.0f,  1.0f}, {0.0f, 0.0f}},
        {{ 1.0f,  1.0f}, {1.0f, 0.0f}},
        {{ 1.0f, -1.0f}, {1.0f, 1.0f}},
        {{-1.0f,  1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f}, {1.0f, 1.0f}},
        {{-1.0f, -1.0f}, {0.0f, 1.0f}},
    };

    _screenQuad = std::make_shared<MeshObject>(std::vector<uint8_t>{(uint8_t*)sqVertices.data(), (uint8_t*)(sqVertices.data() + sqVertices.size())},
                                               sizeof(screenQuadVertex),
                                               std::vector<uint32_t>{},
                                               _device, false);

    return _screenQuad;
}

std::shared_ptr<MeshObject> MeshManager::CreateCustomObject(const std::vector<GeometryVertex>& vertices,
                                                            const std::vector<uint32_t>&       indices,
                                                            std::function<void(CommandList&)>  cmdListExecutor)
{
    _customObjects.emplace_back(std::make_shared<MeshObject>(
        std::span<const uint8_t>{(uint8_t*)vertices.data(), vertices.size() * sizeof(GeometryVertex)},
        sizeof(GeometryVertex), indices, _device, true, cmdListExecutor));

    return _customObjects.back();
}
