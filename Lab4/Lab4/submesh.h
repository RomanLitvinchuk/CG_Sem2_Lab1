#ifndef SUBMESH_H_
#define SUBMESH_H_

#include <d3d12.h>
#include <DirectXCollision.h>

struct Submesh
{
    UINT indexCount = 0;
    UINT startIndiceIndex = 0;
    UINT startVerticeIndex = 0;
    int materialIndex = -1;
    DirectX::BoundingBox box;
};
#endif
