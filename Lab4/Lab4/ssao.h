#ifndef SSAO_H_
#define SSAO_H_
#include <d3d12.h>
#include <SimpleMath.h>
#include "throw_if_failed.h"
#include "texture.h"
#include "d3dx12.h"

class SSAO {
private:
	MyTexture SSAOTexture_A;
	MyTexture SSAOTexture_B;
	ComPtr<ID3D12DescriptorHeap> srvHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap> samplerHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap> rtvHeap = nullptr;

	void CreateHeaps(ComPtr<ID3D12Device> device);
	void CreateTexture(ComPtr<ID3D12Device> device, int width, int height);
	void CreateRTV(ComPtr<ID3D12Device> device);
	void CreateSRV(ComPtr<ID3D12Device> device, ID3D12Resource* depthTexture, ID3D12Resource* normalTexture, ID3D12Resource* noiseTexture);
	void CreateSamplers(ComPtr<ID3D12Device> device);

	void BarriersToDefault(ComPtr<ID3D12GraphicsCommandList> commandList);

public:
	SSAO(ComPtr<ID3D12Device> device, int width, int height, ID3D12Resource* depthTexture, ID3D12Resource* normalTexture, ID3D12Resource* noiseTexture) {
		CreateHeaps(device);
		CreateTexture(device, width, height);
		CreateRTV(device);
		CreateSRV(device, depthTexture, normalTexture, noiseTexture);
		CreateSamplers(device);
	}
	void OnResize(ComPtr<ID3D12Device> device, int width, int height, ID3D12Resource* depthTexture, ID3D12Resource* normalTexture, ID3D12Resource* noiseTexture);
	void ClearSSAO(ComPtr<ID3D12GraphicsCommandList> commandList);

	MyTexture& GetTextureA(){
		return SSAOTexture_A;
	}

	MyTexture& GetTextureB() {
		return SSAOTexture_B;
	}

	ComPtr<ID3D12DescriptorHeap> GetSrvHeap() {
		return srvHeap;
	}

	ComPtr<ID3D12DescriptorHeap> GetSamplerHeap() {
		return samplerHeap;
	}

	ComPtr<ID3D12DescriptorHeap> GetRtvHeap() {
		return rtvHeap;
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
