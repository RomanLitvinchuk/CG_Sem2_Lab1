#ifndef RENDERING_SYSTEM_
#define RENDERING_SYSTEM_

#include <d3d12.h>
#include <wrl.h>
#include "g_buffer.h"
#include "light.h"
#include "post_process.h"
#include <SimpleMath.h>
#include "ssao.h"
#include "singletone_device.h"

using namespace Microsoft::WRL;
using namespace DirectX::SimpleMath;

class RenderingSystem {
private:
	ComPtr<ID3D12Device> device = SingletonDevice::GetDevice();

	void BuildLayouts();
	void CompileShaders();

	void CreateOpaqueRS();
	void CreateOpaquePSO(std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);

	void CreateLightRS();
	void CreateLightPSO();

	void CreateBulbRS();
	void CreateBulbPSO(std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);

	void CreateStreamOutputRS();
	void CreateStreamOutputPSO(std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);
	void CreateBakedPSO(std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);

	void CreateWireframeRS();
	void CreateWireframePSO(std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);

	void CreateParticleRS();
	void CreateParticlePSO();

	void CreateParticlesUpdateRS();
	void CreateParticlesUpdatePSO();

	void CreateParticlesEmitRS();
	void CreateParticlesEmitPSO();

	void CreateShadowRS();
	void CreateShadowPSO(std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);

	void CreateSSAORS();
	void CreateSSAOPSO();

	void CreateSSAOBlurRS();
	void CreateSSAOBlurPSO();

	void CreateBillboardRS();
	void CreateBillboardPSO();

	void CreatePPDefaultRS();
	void CreatePPTonemappingPSO();

	void CreatePPVignettePSO();

	void CreatePPOutputPSO();

	void GenerateTreeLights(std::vector<LightConstants>& lightsArray, Vector3 treeBasePosition, float treeHeight, float treeBaseRadius, int count);

public:
	ComPtr<ID3D12RootSignature> opaqueRS_ = nullptr;
	ComPtr<ID3D12RootSignature> lightRS_ = nullptr;

	ComPtr<ID3D12PipelineState> opaquePSO_ = nullptr;
	ComPtr<ID3D12PipelineState> lightPSO_ = nullptr;

	ComPtr<ID3DBlob> opaqueVS_ = nullptr;
	ComPtr<ID3DBlob> opaquePS_ = nullptr;

	ComPtr<ID3DBlob> fullscreenTriangleVS_ = nullptr;
	ComPtr<ID3DBlob> lightPS_ = nullptr;

	ComPtr<ID3D12RootSignature> bulbRS_ = nullptr;
	ComPtr<ID3D12PipelineState> bulbPSO_ = nullptr;
	ComPtr<ID3DBlob> bulbVS_ = nullptr;
	ComPtr<ID3DBlob> bulbPS_ = nullptr;

	ComPtr<ID3DBlob> HS_ = nullptr;
	ComPtr<ID3DBlob> DS_ = nullptr;

	ComPtr<ID3DBlob> tessVS_ = nullptr;

	ComPtr<ID3D12RootSignature> streamOutputRS_ = nullptr;
	ComPtr<ID3D12PipelineState> streamOutputPSO_ = nullptr;

	ComPtr<ID3D12PipelineState> bakedPSO_ = nullptr;
	ComPtr<ID3DBlob> bakedVS_ = nullptr;

	ComPtr<ID3D12RootSignature> wireframeRS_;
	ComPtr<ID3D12PipelineState> wireframePSO_;
	ComPtr<ID3DBlob> wireframeVS_ = nullptr;
	ComPtr<ID3DBlob> wireframePS_ = nullptr;

	ComPtr<ID3D12RootSignature> particleRS_ = nullptr;
	ComPtr<ID3D12PipelineState> particlePSO_ = nullptr;
	ComPtr<ID3DBlob> particleVS_ = nullptr;
	ComPtr<ID3DBlob> particleGS_ = nullptr;
	ComPtr<ID3DBlob> particlePS_ = nullptr;

	ComPtr<ID3D12RootSignature> particlesUpdateRS_ = nullptr;
	ComPtr<ID3D12PipelineState> particlesUpdatePSO_ = nullptr;
	ComPtr<ID3DBlob> particleUpdateCS_ = nullptr;

	ComPtr<ID3D12RootSignature> particlesEmitRS_ = nullptr;
	ComPtr<ID3D12PipelineState> particlesEmitPSO_ = nullptr;
	ComPtr<ID3DBlob> particleEmitCS_ = nullptr;

	ComPtr<ID3D12RootSignature> shadowRS_ = nullptr;
	ComPtr<ID3D12PipelineState> shadowPSO_ = nullptr;
	ComPtr<ID3DBlob> shadowVS_ = nullptr;

	ComPtr<ID3D12RootSignature> SsaoRS_ = nullptr;
	ComPtr<ID3D12PipelineState> SsaoPSO_ = nullptr;
	ComPtr<ID3DBlob> SsaoPS_ = nullptr;

	ComPtr<ID3D12RootSignature> SsaoBlurRS_ = nullptr;
	ComPtr<ID3D12PipelineState> SsaoBlurPSO_ = nullptr;
	ComPtr<ID3DBlob> SsaoBlurPS_ = nullptr;

	ComPtr<ID3D12RootSignature> billboardRS_ = nullptr;
	ComPtr<ID3D12PipelineState> billboardPSO_ = nullptr;
	ComPtr<ID3DBlob> billboardVS_ = nullptr;
	ComPtr<ID3DBlob> billboardPS_ = nullptr;

	ComPtr<ID3D12RootSignature> pp_defaultRS_ = nullptr;
	ComPtr<ID3D12PipelineState> pp_tonemappingPSO_ = nullptr;
	ComPtr<ID3DBlob> pp_tonemappingPS_ = nullptr;

	ComPtr<ID3D12PipelineState> pp_vignettePSO_ = nullptr;
	ComPtr<ID3DBlob> pp_vignettePS_ = nullptr;

	ComPtr<ID3D12PipelineState> pp_outputPSO_ = nullptr;
	ComPtr<ID3DBlob> pp_outputPS_ = nullptr;


	std::unique_ptr<GBuffer> g_buffer = nullptr;
	std::unique_ptr<PostProcess> post_process = nullptr;
	std::unique_ptr<SSAO> ssao = nullptr;

	std::vector<LightConstants> sceneLights_;
	ComPtr<ID3D12DescriptorHeap> samplerHeap = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout_;
	std::vector<D3D12_INPUT_ELEMENT_DESC> bakedLayout_;
	std::vector<D3D12_INPUT_ELEMENT_DESC> wireframeLayout_;

	RenderingSystem(int width, int height, ID3D12Resource* noiseTexture) {
		BuildLayouts();
		CreateOpaqueRS();
		CompileShaders();
		CreateOpaquePSO(inputLayout_);

		CreateStreamOutputRS();
		CreateStreamOutputPSO(inputLayout_);
		CreateBakedPSO(bakedLayout_);

		CreateLightRS();
		CreateLightPSO();

		CreateWireframeRS();
		CreateWireframePSO(wireframeLayout_);

		CreateParticleRS();
		CreateParticlePSO();

		CreateParticlesUpdateRS();
		CreateParticlesUpdatePSO();
		CreateParticlesEmitRS();
		CreateParticlesEmitPSO();

		CreateShadowRS();
		CreateShadowPSO(inputLayout_);

		CreateSSAORS();
		CreateSSAOPSO();

		CreateSSAOBlurRS();
		CreateSSAOBlurPSO();

		CreateBillboardRS();
		CreateBillboardPSO();

		g_buffer = std::make_unique<GBuffer>(width, height);
		post_process = std::make_unique<PostProcess>(width, height);
		ssao = std::make_unique<SSAO>(width / 2, height / 2, g_buffer->GetDepthTex().Resource.Get(), g_buffer->GetNormalTex().Resource.Get(), noiseTexture);

		LightConstants sun = {};
		sun.lightType = 0; // Directional
		sun.lightDirection = { 0.0f, -1.0f, 0.0f };
		sun.lightColor = { 1.0f, 0.9f, 0.8f };
		sceneLights_.push_back(sun);

		GenerateTreeLights(sceneLights_, { 10.0f, 0.0f, -60.0f }, 350.0f, 100.0f, 500);

		CreateBulbRS();
		CreateBulbPSO(inputLayout_);

		CreatePPDefaultRS();
		CreatePPTonemappingPSO();
		CreatePPVignettePSO();
		CreatePPOutputPSO();



		D3D12_DESCRIPTOR_HEAP_DESC sampHeapDesc = {};
		sampHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		sampHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		sampHeapDesc.NumDescriptors = 2;
		ThrowIfFailed(device->CreateDescriptorHeap(&sampHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&samplerHeap));

		D3D12_SAMPLER_DESC sampDesc = {};
		sampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		sampDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampDesc.MinLOD = 0;
		sampDesc.MaxLOD = D3D12_FLOAT32_MAX;
		sampDesc.MipLODBias = 0.0f;
		sampDesc.MaxAnisotropy = 1;
		sampDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		device->CreateSampler(&sampDesc, samplerHeap->GetCPUDescriptorHandleForHeapStart());

		sampDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
		sampDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		sampDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampDesc.BorderColor[0] = 1.0f;
		sampDesc.BorderColor[1] = 1.0f;
		sampDesc.BorderColor[2] = 1.0f;
		sampDesc.BorderColor[3] = 1.0f;

		auto handle = samplerHeap->GetCPUDescriptorHandleForHeapStart();
		auto size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		CD3DX12_CPU_DESCRIPTOR_HANDLE smpHandle(handle, 1, size);
		device->CreateSampler(&sampDesc, smpHandle);
	}
};


#endif //RENDERING_SYSTEM_
