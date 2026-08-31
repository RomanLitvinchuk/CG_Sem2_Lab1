#ifndef POST_PROCESS_
#define POST_PROCESS_
#include <d3d12.h>
#include <d3dx12.h>
#include <wrl.h>
#include <SimpleMath.h>
#include "throw_if_failed.h"
#include "texture.h"


using namespace Microsoft::WRL;
using namespace DirectX;

struct PostProcess {
	MyTexture HDR_Texture_A;
	MyTexture HDR_Texture_B;
	MyTexture LDR_Texture_A;
	MyTexture LDR_Texture_B;
	ComPtr<ID3D12DescriptorHeap> srvHeap_;
	ComPtr<ID3D12DescriptorHeap> rtvHeap_;

	void CreateTexture(int width, int height, ComPtr<ID3D12Device> device);
	void CreateSRV(ComPtr<ID3D12Device> device);
	void CreateRTV(ComPtr<ID3D12Device> device);

	void ClearPostProcess(ComPtr<ID3D12GraphicsCommandList> commandList);
	void BarriersToDefault(ComPtr<ID3D12GraphicsCommandList> commandList);
	void OnResize(int width, int height, ComPtr<ID3D12Device> device);

	PostProcess(int width, int height, ComPtr<ID3D12Device> device) {
		D3D12_DESCRIPTOR_HEAP_DESC descHeap = {};
		descHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		descHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		descHeap.NumDescriptors = 4;
		ThrowIfFailed(device->CreateDescriptorHeap(&descHeap, IID_PPV_ARGS(&srvHeap_)));

		descHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		descHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(device->CreateDescriptorHeap(&descHeap, IID_PPV_ARGS(&rtvHeap_)));

		CreateTexture(width, height, device);
		CreateSRV(device);
		CreateRTV(device);
	}
};


#endif POST_PROCESS_
