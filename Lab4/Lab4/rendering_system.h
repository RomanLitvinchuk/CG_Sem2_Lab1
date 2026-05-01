#ifndef RENDERING_SYSTEM_
#define RENDERING_SYSTEM_

#include <d3d12.h>
#include <wrl.h>
#include "g_buffer.h"
#include "light.h"
#include <SimpleMath.h>

using namespace Microsoft::WRL;
using namespace DirectX::SimpleMath;

struct RenderingSystem {
	ComPtr<ID3D12RootSignature> opaqueRS_ = nullptr;
	ComPtr<ID3D12RootSignature> lightRS_ = nullptr;

	ComPtr<ID3D12PipelineState> opaquePSO_ = nullptr;
	ComPtr<ID3D12PipelineState> lightPSO_ = nullptr;

	ComPtr<ID3DBlob> opaqueVS_ = nullptr;
	ComPtr<ID3DBlob> opaquePS_ = nullptr;

	ComPtr<ID3DBlob> lightVS_ = nullptr;
	ComPtr<ID3DBlob> lightPS_ = nullptr;

	ComPtr<ID3D12RootSignature> bulbRS_ = nullptr;
	ComPtr<ID3D12PipelineState> bulbPSO_ = nullptr;
	ComPtr<ID3DBlob> bulbVS_ = nullptr;
	ComPtr<ID3DBlob> bulbPS_ = nullptr;

	ComPtr<ID3DBlob> HS_ = nullptr;
	ComPtr<ID3DBlob> DS_ = nullptr;

	ComPtr<ID3D12PipelineState> tessPSO_ = nullptr;
	ComPtr<ID3DBlob> tessPS_ = nullptr;
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

	GBuffer* g_buffer = nullptr;

	std::vector<LightConstants> sceneLights_;
	ComPtr<ID3D12DescriptorHeap> samplerHeap = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout_;
	std::vector<D3D12_INPUT_ELEMENT_DESC> bakedLayout_;
	std::vector<D3D12_INPUT_ELEMENT_DESC> wireframeLayout_;

	void BuildLayouts();
	void CompileShaders();

	void CreateOpaqueRS(ComPtr<ID3D12Device> device);
	void CreateOpaquePSO(ComPtr<ID3D12Device> device, std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);

	void CreateLightRS(ComPtr<ID3D12Device> device);
	void CreateLightPSO(ComPtr<ID3D12Device>, std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);

	void CreateBulbRS(ComPtr<ID3D12Device> device);
	void CreateBulbPSO(ComPtr<ID3D12Device> device, std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);

	void CreateTessPSO(ComPtr<ID3D12Device> device, std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);

	void CreateStreamOutputRS(ComPtr<ID3D12Device> device);
	void CreateStreamOutputPSO(ComPtr<ID3D12Device> device, std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);
	void CreateBakedPSO(ComPtr<ID3D12Device> device, std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);

	void CreateWireframeRS(ComPtr<ID3D12Device> device);
	void CreateWireframePSO(ComPtr <ID3D12Device> device, std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);

	void CreateParticleRS(ComPtr<ID3D12Device> device);
	void CreateParticlePSO(ComPtr<ID3D12Device> device);

	void CreateParticlesUpdateRS(ComPtr<ID3D12Device> device);
	void CreateParticlesUpdatePSO(ComPtr<ID3D12Device> device);

	void CreateParticlesEmitRS(ComPtr<ID3D12Device> device);
	void CreateParticlesEmitPSO(ComPtr<ID3D12Device> device);

	void CreateShadowRS(ComPtr<ID3D12Device> device);
	void CreateShadowPSO(ComPtr<ID3D12Device> device, std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);

	void GenerateTreeLights(std::vector<LightConstants>& lightsArray, Vector3 treeBasePosition, float treeHeight, float treeBaseRadius, int count);

	RenderingSystem(ComPtr<ID3D12Device> device, int width, int height) {
		BuildLayouts();
		CreateOpaqueRS(device);
		CompileShaders();
		CreateOpaquePSO(device, inputLayout_);

		CreateStreamOutputRS(device);
		CreateStreamOutputPSO(device, inputLayout_);
		CreateBakedPSO(device, bakedLayout_);

		CreateLightRS(device);
		CreateLightPSO(device, inputLayout_);

		CreateWireframeRS(device);
		CreateWireframePSO(device, wireframeLayout_);

		CreateParticleRS(device);
		CreateParticlePSO(device);

		CreateParticlesUpdateRS(device);
		CreateParticlesUpdatePSO(device);
		CreateParticlesEmitRS(device);
		CreateParticlesEmitPSO(device);

		CreateShadowRS(device);
		CreateShadowPSO(device, inputLayout_);

		g_buffer = new GBuffer(width, height, device);

		LightConstants sun = {};
		sun.lightType = 0; // Directional
		sun.lightDirection = { 0.0f, -1.0f, 0.0f };
		sun.lightColor = { 1.0f, 0.9f, 0.8f };
		sceneLights_.push_back(sun);

		GenerateTreeLights(sceneLights_, { 10.0f, 0.0f, -60.0f }, 350.0f, 100.0f, 500);

		CreateBulbRS(device);
		CreateBulbPSO(device, inputLayout_);

		//CreateTessPSO(device, layout);


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

		sampDesc.Filter = D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT;
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
