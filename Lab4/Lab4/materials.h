#ifndef MATERIALS_H_
#define NATERIALS_H_
#include <d3d12.h>
#include <SimpleMath.h>
#include <string>

using namespace DirectX;
using namespace SimpleMath;

struct MaterialConstants
{
    Vector4 DiffuseColor;
    Vector4 AmbientColor;
    Vector4 SpecularColor;
    Vector4 EmissiveColor;
    Vector4 TransparentColor;

    float Shininess;
    float Opacity;
    float RefractionIndex;



    Matrix MatTransform = Matrix::Identity;
};



#endif //MATERIALS_H_
