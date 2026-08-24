#ifndef SSAO_H_
#define SSAO_H_
#include <d3d12.h>
#include <SimpleMath.h>
#include "throw_if_failed.h"
#include "post_process.h"

struct SSAO {
	PPTexture SSAOTexture_;
	PPTexture NoiseTexture_;
	ComPtr<ID3D12DescriptorHeap> srvHeap_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> rtvHeap_ = nullptr;

	void CreateTexture(ComPtr<ID3D12Device> device, int width, int height);
	void CreateRTV(ComPtr<ID3D12Device> device);
	void CreateSRV(ComPtr<ID3D12Device> device);

	SSAO(ComPtr<ID3D12Device> device, int width, int height) {
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.NumDescriptors = 2;
		srvHeapDesc.NodeMask = 0;

		ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap_)));

		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.NumDescriptors = 1;
		rtvHeapDesc.NodeMask = 0;

		ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap_)));

		CreateTexture(device, width, height);
		CreateRTV(device);
		CreateSRV(device);
	}
};







#endif //SSAO_H_
