#include "post_process.h"

void PostProcess::CreateTexture(int width, int height, ComPtr<ID3D12Device> device) {
	D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_CLEAR_VALUE clearValue;
	clearValue.Color[0] = 0.0f;
	clearValue.Color[1] = 0.0f;
	clearValue.Color[2] = 0.0f;
	clearValue.Color[3] = 1.0f;
	clearValue.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearValue, IID_PPV_ARGS(&ppTexture_.Resource)));
	ppTexture_.Resource->SetName(L"Post Process Texture");
} 

void PostProcess::CreateSRV(ComPtr<ID3D12Device> device) {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(srvHeap_->GetCPUDescriptorHandleForHeapStart());
	ppTexture_.srvHandle = srvHandle;
	device->CreateShaderResourceView(ppTexture_.Resource.Get(), &srvDesc, srvHandle);
}

void PostProcess::CreateRTV(ComPtr<ID3D12Device> device)
{
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap_->GetCPUDescriptorHandleForHeapStart());
	ppTexture_.rtvHandle = rtvHandle;
	device->CreateRenderTargetView(ppTexture_.Resource.Get(), &rtvDesc, rtvHandle);
}

void PostProcess::onResize(int width, int height, ComPtr<ID3D12Device> device)
{
	ppTexture_.Resource.Reset();

	CreateTexture(width, height, device);
	CreateSRV(device);
	CreateRTV(device);
}
