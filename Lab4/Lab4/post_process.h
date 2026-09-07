#ifndef POST_PROCESS_
#define POST_PROCESS_
#include <d3d12.h>
#include <d3dx12.h>
#include <wrl.h>
#include <SimpleMath.h>
#include "throw_if_failed.h"
#include "texture.h"
#include "singletone_device.h"


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

	ComPtr<ID3D12Device> device = SingletonDevice::GetDevice();

	void CreateHeaps();
	void CreateTextures(int width, int height);
	void CreateSRV();
	void CreateRTV();
	void BarriersToDefault(ComPtr<ID3D12GraphicsCommandList> commandList);
	void ResetTextures();
public:
	void ClearPostProcess(ComPtr<ID3D12GraphicsCommandList> commandList);
	void OnResize(int width, int height);

	PostProcess(int width, int height) {
		CreateHeaps();
		CreateTextures(width, height);
		CreateSRV();
		CreateRTV();
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
