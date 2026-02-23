#ifndef MATERIALS_H_
#define NATERIALS_H_
#include <d3d12.h>
#include <SimpleMath.h>
#include <string>

using namespace DirectX;
using namespace SimpleMath;

struct MaterialConstants
{

    Matrix MatTransform = Matrix::Identity;

    Vector4 DiffuseColor;
    Vector4 AmbientColor;
    Vector4 SpecularColor;
    Vector4 EmissiveColor;
    Vector4 TransparentColor;

    float Shininess;
    float Opacity;
    float RefractionIndex;
    float Padding;

};

//static_assert(sizeof(MaterialConstants) == 160,
//    "MaterialConstants should be 160 bytes (16*5 + 4*3 + 4 + 64)");


#endif //MATERIALS_H_
