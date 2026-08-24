#include "ssao.h"

void SSAO::CreateTexture(ComPtr<ID3D12Device> device, int width, int height) {
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_FLOAT, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_CLEAR_VALUE clearValue;
	clearValue.Format = DXGI_FORMAT_R32_FLOAT;
	clearValue.Color[0] = 0.0f;
	device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue, IID_PPV_ARGS(&SSAOTexture_.Resource));
	SSAOTexture_.Resource->SetName(L"SSAO Texture");
}

void SSAO::CreateRTV(ComPtr<ID3D12Device> device) {
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeap_->GetCPUDescriptorHandleForHeapStart());
	SSAOTexture_.rtvHandle = handle;
	device->CreateRenderTargetView(SSAOTexture_.Resource.Get(), &rtvDesc, handle);
}

void SSAO::CreateSRV(ComPtr<ID3D12Device> device) {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;
	D3D12_CPU_DESCRIPTOR_HANDLE handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(srvHeap_->GetCPUDescriptorHandleForHeapStart());
	SSAOTexture_.srvHandle = handle;
	device->CreateShaderResourceView(SSAOTexture_.Resource.Get(), &srvDesc, handle);
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(srvHeap_->GetGPUDescriptorHandleForHeapStart());
	SSAOTexture_.srvGpuHandle = gpuHandle;
}