#ifndef CAMERA_H_
#define CAMERA_H_
#include <SimpleMath.h>



struct HullBuffer {
	DirectX::SimpleMath::Vector3 CameraPos;
	float gMinTess;

	float gMaxTess;
	float gMinDist;
	float gMaxDist;
	float padding;
};

#endif //CAMERA_H_