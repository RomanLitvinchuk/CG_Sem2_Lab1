#ifndef MATERIALS_H_
#define NATERIALS_H_
#include <d3d12.h>
#include <SimpleMath.h>
#include <string>

using namespace DirectX;
using namespace SimpleMath;


struct Material {
	std::string name_;
	int matCBindex_ = -1;
	int diffuseSRVHeapDesc_ = -1;
	Vector4 diffuseAlbedo_ = { 1.0f, 1.0f, 1.0f, 1.0f };
	Vector3 fresnelR0 = { 0.01f, 0.01f, 0.01f };
	float roughness_ = 0.25f;

	Matrix matTransform_ = Matrix::Identity;
	
};



#endif //MATERIALS_H_
