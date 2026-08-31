#ifndef SSAO_H_
#define SSAO_H_
#include <d3d12.h>
#include <SimpleMath.h>
#include "throw_if_failed.h"
#include "texture.h"
#include "d3dx12.h"

struct SSAO {
	MyTexture SSAOTexture_A;
	MyTexture SSAOTexture_B;
	ComPtr<ID3D12DescriptorHeap> srvHeap_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> samplerHeap_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> rtvHeap_ = nullptr;

	void CreateTexture(ComPtr<ID3D12Device> device, int width, int height);
	void CreateRTV(ComPtr<ID3D12Device> device);
	void CreateSRV(ComPtr<ID3D12Device> device, ID3D12Resource* depthTexture, ID3D12Resource* normalTexture, ID3D12Resource* noiseTexture);
	void CreateSamplers(ComPtr<ID3D12Device> device);
	void OnResize(ComPtr<ID3D12Device> device, int width, int height, ID3D12Resource* depthTexture, ID3D12Resource* normalTexture, ID3D12Resource* noiseTexture);
	void ClearSSAO(ComPtr<ID3D12GraphicsCommandList> commandList);
	void SwapStates(ComPtr<ID3D12Device> device);

	SSAO(ComPtr<ID3D12Device> device, int width, int height, ID3D12Resource* depthTexture, ID3D12Resource* normalTexture, ID3D12Resource* noiseTexture) {
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.NumDescriptors = 5;
		srvHeapDesc.NodeMask = 0;
		ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap_)));

		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.NumDescriptors = 2;
		rtvHeapDesc.NodeMask = 0;
		ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap_)));

		D3D12_DESCRIPTOR_HEAP_DESC samplerDesc;
		samplerDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		samplerDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		samplerDesc.NumDescriptors = 2;
		samplerDesc.NodeMask = 0;
		ThrowIfFailed(device->CreateDescriptorHeap(&samplerDesc, IID_PPV_ARGS(&samplerHeap_)));

		CreateTexture(device, width, height);
		CreateRTV(device);
		CreateSRV(device, depthTexture, normalTexture, noiseTexture);
		CreateSamplers(device);
	}
};

struct SsaoConstants
{
	float screenWidth;
	float screenHeight;
	float randomTextureSize;
	float sampleRadius;
	float ssaoScale;
	float ssaoBias;
	float ssaoIntensity;
	float padding;
};

struct BlurConstants {
	float screenWidth;
	float screenHeight;
	float blurType; //0.0f - horizontal blur, else - vertical blur
	float padding;
};







#endif //SSAO_H_
