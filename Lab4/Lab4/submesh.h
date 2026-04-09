#ifndef SUBMESH_H_
#define SUBMESH_H_

#include <d3d12.h>
#include <DirectXCollision.h>
#include <SimpleMath.h>
#include <string>

using namespace DirectX;
using namespace SimpleMath;

struct InstanceData {
    Matrix World_;
    Matrix TexTransform_;
    Matrix InvTWorld_;
};

struct Submesh
{
    std::string name_;
    UINT indexCount = 0;
    UINT startIndiceIndex = 0;
    UINT startVerticeIndex = 0;
    int materialIndex = -1;
    DirectX::BoundingBox box;
    UINT InstanceCount = 1;
    Matrix mWorld;
    Matrix mTexTransform = Matrix::Identity;
    std::vector<InstanceData> instances;
};

#endif
