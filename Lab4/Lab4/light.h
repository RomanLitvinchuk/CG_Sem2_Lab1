#ifndef LIGHT_H_
#define LIGHT_H_

#include <SimpleMath.h>

using namespace DirectX;
using namespace SimpleMath;

struct Light {
	Vector3 strength_;
	float fallOfStart_;

	Vector3 direction_;
	float FallofEnd;

	Vector3 position_;
	float spotPower;

};



#endif //LIGHT_H_
