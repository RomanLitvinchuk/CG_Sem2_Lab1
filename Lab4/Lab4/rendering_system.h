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

	GBuffer* g_buffer = nullptr;

	std::vector<LightConstants> sceneLights_;

	void CreateOpaqueRS(ComPtr<ID3D12Device> device);
	void CreateOpaquePSO(ComPtr<ID3D12Device> device, std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);
	void CompileShaders();
	void CreateLightRS(ComPtr<ID3D12Device> device);
	void CreateLightPSO(ComPtr<ID3D12Device>, std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);

	RenderingSystem(ComPtr<ID3D12Device> device, std::vector<D3D12_INPUT_ELEMENT_DESC>& layout, int width, int height) {
		CreateOpaqueRS(device);
		CompileShaders();
		CreateOpaquePSO(device, layout);
		CreateLightRS(device);
		CreateLightPSO(device, layout);
		g_buffer = new GBuffer(width, height, device);

		LightConstants sun = {};
		sun.lightType = 0; // Directional
		sun.lightDirection = { 0.5f, -1.0f, 0.5f };
		sun.lightColor = { 1.0f, 0.9f, 0.8f };
		sceneLights_.push_back(sun);

		LightConstants bulb = {};
		bulb.lightType = 1; // Point
		bulb.lightPosition = { 10.0f, 5.0f, 0.0f };
		bulb.lightRange = 20.0f;
		bulb.lightColor = { 1.0f, 0.0f, 0.0f };
		sceneLights_.push_back(bulb);

		LightConstants flashLight = {};
		flashLight.lightType = 2;
		flashLight.lightColor = { 1.0f, 1.0f, 1.0f };
		flashLight.lightPosition = { 0.0f, 2.0f, 0.0f };
		flashLight.lightDirection = { 0.0f, 0.0f, 1.0f };
		flashLight.lightRange = 50.0f;
		flashLight.SpotCosInner = cosf(DirectX::XMConvertToRadians(15.0f));
		flashLight.SpotCosOuter = cosf(DirectX::XMConvertToRadians(30.0f));
		sceneLights_.push_back(flashLight);
	}
};


#endif //RENDERING_SYSTEM_
