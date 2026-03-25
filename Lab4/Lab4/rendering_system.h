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

	GBuffer* g_buffer = nullptr;

	std::vector<LightConstants> sceneLights_;
	ComPtr<ID3D12DescriptorHeap> samplerHeap = nullptr;

	void CreateOpaqueRS(ComPtr<ID3D12Device> device);
	void CreateOpaquePSO(ComPtr<ID3D12Device> device, std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);
	void CompileShaders();
	void CreateLightRS(ComPtr<ID3D12Device> device);
	void CreateLightPSO(ComPtr<ID3D12Device>, std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);

	void CreateBulbRS(ComPtr<ID3D12Device> device);
	void CreateBulbPSO(ComPtr<ID3D12Device> device, std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);

	void GenerateTreeLights(std::vector<LightConstants>& lightsArray, Vector3 treeBasePosition, float treeHeight, float treeBaseRadius, int count);

	RenderingSystem(ComPtr<ID3D12Device> device, std::vector<D3D12_INPUT_ELEMENT_DESC>& layout, int width, int height) {
		CreateOpaqueRS(device);
		CompileShaders();
		CreateOpaquePSO(device, layout);

		CreateLightRS(device);
		CreateLightPSO(device, layout);

		g_buffer = new GBuffer(width, height, device);

		LightConstants sun = {};
		sun.lightType = 0; // Directional
		sun.lightDirection = { 0.0f, 0.0f, 1.0f };
		sun.lightColor = { 1.0f, 0.9f, 0.8f };
		sceneLights_.push_back(sun);

		GenerateTreeLights(sceneLights_, { 10.0f, 0.0f, -60.0f }, 350.0f, 100.0f, 500);

		CreateBulbRS(device);
		CreateBulbPSO(device, layout);


		D3D12_DESCRIPTOR_HEAP_DESC sampHeapDesc = {};
		sampHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		sampHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		sampHeapDesc.NumDescriptors = 1;
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
	}
};


#endif //RENDERING_SYSTEM_
