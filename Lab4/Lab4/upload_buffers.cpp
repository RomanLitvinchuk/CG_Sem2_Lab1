#include "DX12App.h"

void DX12App::InitUploadBuffers() {
	CBUploadBuffer = std::make_unique<UploadBuffer<ObjectConstants>>(
		m_device_.Get(),
		1,
		true
	);
	MatricesBuffer = std::make_unique<UploadBuffer<Matrices>>(m_device_.Get(), 1, true);
	ParticleConstantsBuffer = std::make_unique<UploadBuffer<ParticleConstants>>(m_device_.Get(), 1, true);
	MaterialCB = std::make_unique<UploadBuffer<MaterialConstants>>(m_device_.Get(), 300, true);
	CameraCB = std::make_unique<UploadBuffer<CameraConstants>>(m_device_.Get(), 1, true);
	LightBuffer = std::make_unique<UploadBuffer<LightConstants>>(m_device_.Get(), 1000, false);
	InstanceBuffer = std::make_unique<UploadBuffer<MeshInstanceData>>(m_device_.Get(), 1000, false);
	HullCB = std::make_unique<UploadBuffer<HullBuffer>>(m_device_.Get(), 1, true);
	WireframeInstanceBuffer = std::make_unique<UploadBuffer<WireframeInstanceData>>(m_device_.Get(), 1000, false);
	ShadowCB = std::make_unique<UploadBuffer<ShadowConstants>>(m_device_.Get(), 3, true);
	SsaoBuffer = std::make_unique<UploadBuffer<SsaoConstants>>(m_device_.Get(), 1, true);

	DeadListUpload_ = std::make_unique<UploadBuffer<uint32_t>>(m_device_.Get(), PARTICLE_COUNT, false);
	deadCounterUpload_ = std::make_unique<UploadBuffer<uint32_t>>(m_device_.Get(), 1, false);
	sortCounterUpload_ = std::make_unique<UploadBuffer<uint32_t>>(m_device_.Get(), 1, false);

	FillUploadBuffers();
}

void DX12App::FillUploadBuffers()
{
	SsaoConstants ssaoConst;
	ssaoConst.screenWidth = m_client_width_;
	ssaoConst.screenHeight = m_client_height_;
	ssaoConst.randomTextureSize = 64;
	ssaoConst.sampleRadius = 1.0f;
	ssaoConst.ssaoScale = 1.0f;
	ssaoConst.ssaoBias = 0.1f;
	ssaoConst.ssaoIntensity = 2.0f;
	ssaoConst.padding = 0.0f;
	SsaoBuffer->CopyData(0, ssaoConst);

	for (int i = 0; i < PARTICLE_COUNT; i++) {
		DeadListUpload_->CopyData(i, i);
	}
	deadCounterUpload_->CopyData(0, PARTICLE_COUNT);
	sortCounterUpload_->CopyData(0, 0);
}
