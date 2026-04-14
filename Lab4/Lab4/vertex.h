#ifndef VERTEX_H_
#define VERTEX_H_
#include <SimpleMath.h>
#include <d3d12.h>
#include <fstream>
#include <sstream>

using namespace DirectX;
using namespace SimpleMath;

struct Vertex
{
    Vector3 pos;
    Vector3 normal;
    Vector2 uv;
    Vector3 tangent;
    Vector3 biNormal;
};

struct BakedVertex
{
    Vector3 worldPos;
    Vector3 normal;
    Vector3 normalW;
    Vector3 uv;
    Vector3 tangentW;
};

#endif //VERTEX_H_