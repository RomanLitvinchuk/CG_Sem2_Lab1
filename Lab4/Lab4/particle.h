#ifndef PARTICLE_H_
#define PARTICLE_H_
#include <SimpleMath.h>

using namespace DirectX::SimpleMath;

struct Particle {
	Vector4 Position;
	Vector3 Velocity;
	float padding;
	Vector4 Color;
	float age = 0;
	uint32_t isAlive = 1;
	float pad[2];
};


#endif //PARTICLE_H_
