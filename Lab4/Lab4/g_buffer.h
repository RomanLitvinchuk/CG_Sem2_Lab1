#ifndef G_BUFFER_
#define G_BUFFER_

#include <d3d12.h>
#include <d3dx12.h>
#include <wrl.h>
#include <SimpleMath.h>
#include <DirectXColors.h>
#include "throw_if_failed.h"

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace SimpleMath;

struct GBufferTexture {
	ComPtr<ID3D12Resource> Resource = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = {};
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = {};
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};
};


struct GBuffer {
	GBufferTexture DiffuseTex;
	GBufferTexture NormalTex;
	GBufferTexture DepthTex;

	ComPtr<ID3D12DescriptorHeap> RTVDescriptorHeap;
	ComPtr<ID3D12DescriptorHeap> SRVDescriptorHeap;
	ComPtr<ID3D12DescriptorHeap> DSVDescriptorHeap;

	void CreateTextures(int width, int height, ComPtr<ID3D12Device> device);
	void CreateSRV(ComPtr<ID3D12Device> device);
	void CreateRTVandDSV(ComPtr<ID3D12Device> device);

	GBuffer(int width, int height, ComPtr<ID3D12Device> device) {
		D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {};
		rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		rtvDesc.NumDescriptors = 2;
		rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		ThrowIfFailed(device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&RTVDescriptorHeap)));
		
		D3D12_DESCRIPTOR_HEAP_DESC srvDesc = {};
		srvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		srvDesc.NumDescriptors = 4;
		srvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		ThrowIfFailed(device->CreateDescriptorHeap(&srvDesc, IID_PPV_ARGS(&SRVDescriptorHeap)));

		D3D12_DESCRIPTOR_HEAP_DESC dsvDesc = {};
		dsvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		dsvDesc.NumDescriptors = 1;
		dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		ThrowIfFailed(device->CreateDescriptorHeap(&dsvDesc, IID_PPV_ARGS(&DSVDescriptorHeap)));

		CreateTextures(width, height, device);
		CreateSRV(device);
		CreateRTVandDSV(device);
		
	};

	void TransitToOpaqueRenderingState(ComPtr<ID3D12GraphicsCommandList> commandList);
	void TransitToLightsRenderingState(ComPtr<ID3D12GraphicsCommandList> commandList);
	void TransitToBulbRenderingState(ComPtr<ID3D12GraphicsCommandList> commandList);

	void OnResize(int width, int height, ComPtr<ID3D12Device> device);
};


#endif //G_BUFFER_