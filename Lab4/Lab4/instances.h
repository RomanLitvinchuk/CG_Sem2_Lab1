#ifndef INSTANCES_H_
#define INSTANCES_H_
#include <SimpleMath.h>

using namespace DirectX;
using namespace SimpleMath;


struct MeshInstanceData {
    Matrix World_;
    Matrix TexTransform_;
    Matrix InvTWorld_;
};

struct WireframeInstanceData {
    Vector3 center;
    Vector3 extents;
    Vector4 color;
};


#endif //INSTANCES_H_
