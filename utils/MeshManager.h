#pragma once

#include "stdafx.h"

#include "MeshObject.h"

class MeshManager
{
public:
    MeshManager(ComPtr<ID3D12Device5> device);

    std::shared_ptr<MeshObject> CreateCube(std::function<void(CommandList&)> cmdListExecutor);
    std::shared_ptr<MeshObject> CreateEmptyCube(std::function<void(CommandList&)> cmdListExecutor);
    std::shared_ptr<MeshObject> CreatePlane(std::function<void(CommandList&)> cmdListExecutor);
    std::shared_ptr<MeshObject> CreateScreenQuad();

private:
    ComPtr<ID3D12Device5> _device = nullptr;

    std::vector<std::shared_ptr<MeshObject>> _meshes;
    std::shared_ptr<MeshObject>              _screenQuad = nullptr;
    std::shared_ptr<MeshObject>              _plane      = nullptr;
    std::shared_ptr<MeshObject>              _cube       = nullptr;
    std::shared_ptr<MeshObject>              _emptyCube  = nullptr;
};
