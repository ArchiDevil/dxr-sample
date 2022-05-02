#pragma once

#include "stdafx.h"

#include "MeshObject.h"
#include <shaders/Common.h>

class MeshManager
{
public:
    using MeshObjects = std::vector<std::shared_ptr<MeshObject>>;

    MeshManager(ComPtr<ID3D12Device5> device);

    std::shared_ptr<MeshObject> CreateCube(std::function<void(CommandList&)> cmdListExecutor);
    std::shared_ptr<MeshObject> CreateEmptyCube(std::function<void(CommandList&)> cmdListExecutor);
    std::shared_ptr<MeshObject> CreatePlane(std::function<void(CommandList&)> cmdListExecutor);
    std::shared_ptr<MeshObject> CreateAxes(std::function<void(CommandList&)> cmdListExecutor);
    std::shared_ptr<MeshObject> CreateScreenQuad();

    std::shared_ptr<MeshObject> CreateCustomObject(const std::vector<GeometryVertex>& vertices,
                                                   const std::vector<uint32_t>&       indices,
                                                   std::function<void(CommandList&)>  cmdListExecutor);

private:
    ComPtr<ID3D12Device5> _device = nullptr;

    std::vector<std::shared_ptr<MeshObject>> _meshes;

    MeshObjects                              _customObjects;

    std::shared_ptr<MeshObject>              _screenQuad = nullptr;
    std::shared_ptr<MeshObject>              _plane      = nullptr;
    std::shared_ptr<MeshObject>              _cube       = nullptr;
    std::shared_ptr<MeshObject>              _emptyCube  = nullptr;
    std::shared_ptr<MeshObject>              _axes       = nullptr;
};
