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

class PostProcess {

private:
	MyTexture hdrTextureA;
	MyTexture hdrTextureB;
	MyTexture ldrTextureA;
	MyTexture ldrTextureB;
	ComPtr<ID3D12DescriptorHeap> srvHeap;
	ComPtr<ID3D12DescriptorHeap> rtvHeap;

	void CreateHeaps(ComPtr<ID3D12Device> device);
	void CreateTexture(int width, int height, ComPtr<ID3D12Device> device);
	void CreateSRV(ComPtr<ID3D12Device> device);
	void CreateRTV(ComPtr<ID3D12Device> device);
	void BarriersToDefault(ComPtr<ID3D12GraphicsCommandList> commandList);
public:
	void ClearPostProcess(ComPtr<ID3D12GraphicsCommandList> commandList);
	void OnResize(int width, int height, ComPtr<ID3D12Device> device);

	PostProcess(int width, int height, ComPtr<ID3D12Device> device) {
		CreateHeaps(device);
		CreateTexture(width, height, device);
		CreateSRV(device);
		CreateRTV(device);
	}

	MyTexture& GetHdrTextureA() { 
		return hdrTextureA; 
	}

	MyTexture& GetHdrTextureB() { 
		return hdrTextureB; 
	}

	MyTexture& GetLdrTextureA() { 
		return ldrTextureA; 
	}

	MyTexture& GetLdrTextureB() { 
		return ldrTextureB; 
	}

	ComPtr<ID3D12DescriptorHeap> GetSrvHeap() { 
		return srvHeap; 
	}

	ComPtr<ID3D12DescriptorHeap> GetRtvHeap() { 
		return rtvHeap; 
	}

};


#endif POST_PROCESS_
