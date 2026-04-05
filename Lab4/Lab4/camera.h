#ifndef CAMERA_H_
#define CAMERA_H_
#include <SimpleMath.h>
#include <DirectXCollision.h>


struct HullBuffer {
	DirectX::SimpleMath::Vector3 CameraPos;
	float gMinTess;

	float gMaxTess;
	float gMinDist;
	float gMaxDist;
	float padding;
};

struct Camera {
	BoundingFrustum frustum_;
	Matrix projection_;
	float mCameraYaw;
	float mCameraPitch;
	Vector3 mCameraPos = { 15.0f, 20.0f, -90.0f };
	Vector3 mCameraTarget = { 0.0f, 0.0f, 1.0f };
	Vector3 mCameraUp = { 0.0f, 1.0f, 0.0f };
	float mCameraSpeed = 200.0f;
	float mCameraRotationSpeed = 0.3f;
	float mRadius_ = 5.0f;

};

#endif //CAMERA_H_