#include "DX12App.h"

void DX12App::OnMouseDown(HWND hWnd) {
	SetCapture(hWnd);
}

void DX12App::OnMouseUp() {
	ReleaseCapture();
}

void DX12App::Update() {
	camera.UpdateCameraPos(m_key_states, gt);
	camera.UpdateViewMatrix();
	UpdateMatricesData();
	UpdateParticleData();
	UpdateFrustumData();
	UpdateCameraConstants();
	UpdateObjectsBuffer();
	UpdateHullBuffer();
	UpdateTextureAnimation();
	UpdateTreesLights();

	UpdateCascades();
	UpdateShadowData();
}

void DX12App::UpdateMatricesData() {
	Matrices matricesData;
	matricesData.Proj = camera.mProj_.Transpose();
	matricesData.View = camera.mView_.Transpose();
	Matrix invView = camera.mView_.Transpose();
	invView = invView.Invert();
	matricesData.invView = invView;
	Matrix invProj = camera.mProj_.Transpose();
	invProj = invProj.Invert();
	matricesData.invProj = invProj;
	matricesBuffer->CopyData(0, matricesData);
}

void DX12App::UpdateParticleData() {
	ParticleConstants particleData;
	particleData.deltaTime = gt.DeltaTime();
	particleData.CameraPos = camera.mCameraPos;
	particleData.particlesCount = PARTICLE_COUNT;
	particleConstantsBuffer->CopyData(0, particleData);
}

void DX12App::UpdateFrustumData() {
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
}

void DX12App::UpdateObjectsBuffer() {
	ObjectConstants obj;
	Matrix TWorld = camera.mWorld_.Transpose();
	obj.View = camera.mView_.Transpose();
	obj.Proj = camera.mProj_.Transpose();
	obj.gTime = gt.TotalTime();
	objectsUploadBuffer->CopyData(0, obj);
}

void DX12App::UpdateCameraConstants() {
	Matrix ViewProj = camera.mView_ * camera.mProj_;
	Matrix InvViewProj = ViewProj.Invert();
	ViewProj = ViewProj.Transpose();
	InvViewProj = InvViewProj.Transpose();
	CameraConstants camConst;
	camConst.invViewProj = InvViewProj;
	camConst.cameraPos = camera.mCameraPos;
	cameraBuffer->CopyData(0, camConst);
}

void DX12App::UpdateHullBuffer() {
	HullBuffer hullConst;
	hullConst.CameraPos = camera.mCameraPos;
	hullConst.gMinTess = 1;
	hullConst.gMaxTess = 5;
	hullConst.gMinDist = 10.0f;
	hullConst.gMaxDist = 200.0f;
	hullBuffer->CopyData(0, hullConst);
}

void DX12App::UpdateTextureAnimation() {
	for (int i = 0; i < materialData.size(); ++i) {
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
		materialBuffer->CopyData(i, materialData[i]);
	}
}

void DX12App::UpdateTreesLights() {
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
			lightBuffer->CopyData(i, renderSystem->sceneLights_[i]);
		}
	}
}

void DX12App::UpdateShadowData() {
	int numCascades = shadowMap->GetNumCascades();
	for (int i = 0; i < numCascades; ++i) {
		ShadowConstants shadowData = {};
		shadowData.lightViewProj = cascades.viewProjMats[i];
		shadowData.shadowTransform_ = cascades.shadowTransform[i];
		shadowData.cascadeDistances = Vector4(cascades.distances[0], cascades.distances[1], cascades.distances[2], 0.0f);
		shadowBuffer->CopyData(i, shadowData);
	}
}