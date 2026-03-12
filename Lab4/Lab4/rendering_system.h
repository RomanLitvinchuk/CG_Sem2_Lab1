#ifndef RENDERING_SYSTEM_
#define RENDERING_SYSTEM_

#include <d3d12.h>
#include <wrl.h>
#include "g_buffer.h"

using namespace Microsoft::WRL;

struct RenderingSystem {
	ComPtr<ID3D12RootSignature> opaqueRS_ = nullptr;
	ComPtr<ID3D12RootSignature> lightRS_ = nullptr;

	ComPtr<ID3D12PipelineState> opaquePSO_ = nullptr;
	ComPtr<ID3D12PipelineState> lightPSO_ = nullptr;

	ComPtr<ID3DBlob> opaqueVS_ = nullptr;
	ComPtr<ID3DBlob> opaquePS_ = nullptr;

	GBuffer* g_buffer = nullptr;

	void CreateOpaqueRS(ComPtr<ID3D12Device> device);
	void CreateOpaquePSO(ComPtr<ID3D12Device> device, std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);
	void CompileShaders();

	RenderingSystem(ComPtr<ID3D12Device> device, std::vector<D3D12_INPUT_ELEMENT_DESC>& layout, int width, int height) {
		CreateOpaqueRS(device);
		CompileShaders();
		CreateOpaquePSO(device, layout);
		g_buffer = new GBuffer(width, height, device);
	}
};


#endif //RENDERING_SYSTEM_
