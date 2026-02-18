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
};

#endif //VERTEX_H_