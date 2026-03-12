#ifndef LIGHT_H_
#define LIGHT_H_

#include <SimpleMath.h>

using namespace DirectX;
using namespace SimpleMath;

struct LightConstants {
	Vector3 lightColor;
	int lightType;

	Vector3 lightPosition;
	float lightRange;

	Vector3 lightDirection;
	float SpotCosInner;

	float SpotCosOuter;
	Vector3 padding;
};

struct CameraConstants {
	Matrix invViewProj;

	Vector3 cameraPos;
	float padding;
};


#endif //LIGHT_H_
