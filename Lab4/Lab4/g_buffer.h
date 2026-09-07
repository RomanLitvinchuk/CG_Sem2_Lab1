#ifndef G_BUFFER_
#define G_BUFFER_

#include <d3d12.h>
#include <d3dx12.h>
#include <wrl.h>
#include <SimpleMath.h>
#include <DirectXColors.h>
#include "throw_if_failed.h"
#include "singletone_device.h"

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace SimpleMath;

struct GBufferTexture {
	ComPtr<ID3D12Resource> Resource = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = {};
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = {};
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};
};


class GBuffer {
private:

	GBufferTexture diffuseTex;
	GBufferTexture normalTex;
	GBufferTexture depthTex;

	ComPtr<ID3D12DescriptorHeap> rtvDescpritorHeap;
	ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap;
	ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;

	ComPtr<ID3D12Device> device = SingletonDevice::GetDevice();

	void CreateHeaps();
	void CreateTextures(int width, int height);
	void CreateSRV();
	void CreateRTVandDSV();
	void ResetTextures();
public:

	GBuffer(int width, int height) {
		CreateHeaps();
		CreateTextures(width, height);
		CreateSRV();
		CreateRTVandDSV();
	};

	void TransitToOpaqueRenderingState(ComPtr<ID3D12GraphicsCommandList> commandList);
	void TransitToLightsRenderingState(ComPtr<ID3D12GraphicsCommandList> commandList);

	void OnResize(int width, int height);
	void ClearGBuffer(ComPtr<ID3D12GraphicsCommandList> commandList);

	GBufferTexture& GetDiffuseTex() { 
		return diffuseTex; 
	}

	GBufferTexture& GetNormalTex() {
		return normalTex; 
	}

	GBufferTexture& GetDepthTex() { 
		return depthTex; 
	}

	ComPtr<ID3D12DescriptorHeap> GetSrvHeap() { 
		return srvDescriptorHeap; 
	}

	ComPtr<ID3D12DescriptorHeap> GetRtvHeap() { 
		return rtvDescpritorHeap; 
	}

	ComPtr<ID3D12DescriptorHeap> GetDsvHeap() { 
		return dsvDescriptorHeap; 
	}
};


#endif //G_BUFFER_