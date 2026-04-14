#ifndef OBJECT_CONSTANTS_
#define OBJECT_CONSTANTS_
#include <d3d12.h>
#include <SimpleMath.h>

using namespace DirectX;
using namespace DirectX::SimpleMath;


struct ObjectConstants {
	Matrix mViewProj;
	float gTime;
	float pad[3];
};


#endif //OBJECT_CONSTANTS_
