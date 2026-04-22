#include "DX12App.h"

void DX12App::OnMouseDown(HWND hWnd) {
	SetCapture(hWnd);
}

void DX12App::OnMouseUp() {
	ReleaseCapture();
}

void DX12App::Update() {
	camera.UpdateCameraPos(m_key_states, gt);

	camera.mView_ = Matrix::CreateLookAt(camera.mCameraPos, camera.mCameraPos + camera.mCameraTarget, camera.mCameraUp);

	std::cout << "CameraPos:" << camera.mCameraPos.x << " " << camera.mCameraPos.y << " " << camera.mCameraPos.z << std::endl;
	Matrices matricesData;
	matricesData.Proj = camera.mProj_;
	matricesData.View = camera.mView_;
	MatricesBuffer->CopyData(0, matricesData);
	Matrix ViewProj = camera.mView_ * camera.mProj_;

	if (camera.isFrustumCullingEnabled) {
		XMMATRIX M = ViewProj;
		XMFLOAT4X4 m;
		XMStoreFloat4x4(&m, M);

		camera.planes[0] = XMVectorSet(m._14 + m._11, m._24 + m._21, m._34 + m._31, m._44 + m._41);
		camera.planes[1] = XMVectorSet(m._14 - m._11, m._24 - m._21, m._34 - m._31, m._44 - m._41);
		camera.planes[2] = XMVectorSet(m._14 + m._12, m._24 + m._22, m._34 + m._32, m._44 + m._42);
		camera.planes[3] = XMVectorSet(m._14 - m._12, m._24 - m._22, m._34 - m._32, m._44 - m._42);
		camera.planes[4] = XMVectorSet(m._13, m._23, m._33, m._43);
		camera.planes[5] = XMVectorSet(m._14 - m._13, m._24 - m._23, m._34 - m._33, m._44 - m._43);

		for (int i = 0; i < 6; ++i) camera.planes[i] = XMPlaneNormalize(camera.planes[i]);
	}

	Matrix InvViewProj = ViewProj.Invert();
	ViewProj = ViewProj.Transpose();
	InvViewProj = InvViewProj.Transpose();

	ObjectConstants obj;
	Matrix TWorld = camera.mWorld_.Transpose();
	obj.mViewProj = ViewProj;
	obj.gTime = gt.TotalTime();
	CBUploadBuffer->CopyData(0, obj);

	for (int i = 0; i < materialData.size(); ++i) {
		MaterialCB->CopyData(i, materialData[i]);
	}

	CameraConstants camConst;
	camConst.invViewProj = InvViewProj;
	camConst.cameraPos = camera.mCameraPos;
	CameraCB->CopyData(0, camConst);

	HullBuffer HullConst;
	HullConst.CameraPos = camera.mCameraPos;
	HullConst.gMinTess = 1;
	HullConst.gMaxTess = 5;
	HullConst.gMinDist = 10.0f;
	HullConst.gMaxDist = 200.0f;
	HullCB->CopyData(0, HullConst);

	for (int i = 0; i < materialData.size(); ++i) {
		//materialData[i].MatTransform = Matrix::Identity;
		if (materialData[i].isTree == 1) {
			float tu = materialData[i].MatTransform(1, 0);
			float tv = materialData[i].MatTransform(1, 1);
			tu += 0.1f * gt.DeltaTime();
			tv += 0.02f * gt.DeltaTime();

			if (tu >= 1.0f)
				tu -= 1.0f;
			if (tv >= 1.0f)
				tv -= 1.0f;
			materialData[i].MatTransform(1, 0) = tu;
			materialData[i].MatTransform(1, 1) = tv;
		}
		MaterialCB->CopyData(i, materialData[i]);
	}

	static float timeAccumulator = 0.0f;
	timeAccumulator += gt.DeltaTime();

	if (timeAccumulator >= 1.0f) {
		timeAccumulator = 0.0f;

		for (int i = 0; i < renderSystem->sceneLights_.size(); i++) {
			if (renderSystem->sceneLights_[i].lightType == 1) {
				renderSystem->sceneLights_[i].lightColor.x = static_cast<float>(rand()) / RAND_MAX;
				renderSystem->sceneLights_[i].lightColor.y = static_cast<float>(rand()) / RAND_MAX;
				renderSystem->sceneLights_[i].lightColor.z = static_cast<float>(rand()) / RAND_MAX;
			}
			LightBuffer->CopyData(i, renderSystem->sceneLights_[i]);
		}
	}
}