#include "g_buffer.h"

void GBuffer::CreateTextures(int width, int height, ComPtr<ID3D12Device> device) {
	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	D3D12_CLEAR_VALUE clearValue;
	clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	clearValue.Color[0] = 0.0f;
	clearValue.Color[1] = 0.0f;
	clearValue.Color[2] = 0.0f;
	clearValue.Color[3] = 1.0f;
	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
		&clearValue, IID_PPV_ARGS(&DiffuseTex.Resource)));
	DiffuseTex.Resource->SetName(L"Diffuse texture");

	clearValue.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	resDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
		&clearValue, IID_PPV_ARGS(&NormalTex.Resource)));
	NormalTex.Resource->SetName(L"Normal texture");

	clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	resDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
		&clearValue, IID_PPV_ARGS(&DepthTex.Resource)));
	DepthTex.Resource->SetName(L"Depth texture");
}

void GBuffer::CreateRTVandDSV(ComPtr<ID3D12Device> device) {
	D3D12_RENDER_TARGET_VIEW_DESC rtvTexDesc = {};
	rtvTexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	rtvTexDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvTexDesc.Texture2D.MipSlice = 0;
	rtvTexDesc.Texture2D.PlaneSlice = 0;
	auto RtvIncSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	DiffuseTex.rtvHandle = rtvHandle;
	device->CreateRenderTargetView(DiffuseTex.Resource.Get(), &rtvTexDesc, DiffuseTex.rtvHandle);
	rtvTexDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rtvHandle.Offset(1, RtvIncSize);
	NormalTex.rtvHandle = rtvHandle;
	device->CreateRenderTargetView(NormalTex.Resource.Get(), &rtvTexDesc, NormalTex.rtvHandle);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvTexDesc = {};
	dsvTexDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvTexDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvTexDesc.Texture2D.MipSlice = 0;
	auto DsvIncSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(DSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	DepthTex.dsvHandle = dsvHandle;
	device->CreateDepthStencilView(DepthTex.Resource.Get(), &dsvTexDesc, DepthTex.dsvHandle);
}

void GBuffer::CreateSRV(ComPtr<ID3D12Device> device) {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvTexDesc = {};
	srvTexDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvTexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvTexDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvTexDesc.Texture2D.MipLevels = 1;
	auto SrvIncSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(SRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	DiffuseTex.srvHandle = srvHandle;
	device->CreateShaderResourceView(DiffuseTex.Resource.Get(), &srvTexDesc, DiffuseTex.srvHandle);

	srvTexDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	srvHandle.Offset(1, SrvIncSize);
	NormalTex.srvHandle = srvHandle;
	device->CreateShaderResourceView(NormalTex.Resource.Get(), &srvTexDesc, NormalTex.srvHandle);

	srvTexDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvHandle.Offset(1, SrvIncSize);
	DepthTex.srvHandle = srvHandle;
	device->CreateShaderResourceView(DepthTex.Resource.Get(), &srvTexDesc, DepthTex.srvHandle);
}

void GBuffer::TransitToOpaqueRenderingState(ComPtr<ID3D12GraphicsCommandList> commandList) {
	CD3DX12_RESOURCE_BARRIER diffuseBarrier = CD3DX12_RESOURCE_BARRIER::Transition(DiffuseTex.Resource.Get(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	CD3DX12_RESOURCE_BARRIER normalBarrier = CD3DX12_RESOURCE_BARRIER::Transition(NormalTex.Resource.Get(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	CD3DX12_RESOURCE_BARRIER depthBarrier = CD3DX12_RESOURCE_BARRIER::Transition(DepthTex.Resource.Get(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_DEPTH_WRITE);
	D3D12_RESOURCE_BARRIER barriers[] = { diffuseBarrier, normalBarrier, depthBarrier };
	commandList->ResourceBarrier(3, barriers);
}

void GBuffer::TransitToLightsRenderingState(ComPtr<ID3D12GraphicsCommandList> commandList) {
	CD3DX12_RESOURCE_BARRIER diffuseBarrier = CD3DX12_RESOURCE_BARRIER::Transition(DiffuseTex.Resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	CD3DX12_RESOURCE_BARRIER normalBarrier = CD3DX12_RESOURCE_BARRIER::Transition(NormalTex.Resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	CD3DX12_RESOURCE_BARRIER depthBarrier = CD3DX12_RESOURCE_BARRIER::Transition(DepthTex.Resource.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_STATE_DEPTH_READ);
	D3D12_RESOURCE_BARRIER barriers[] = { diffuseBarrier, normalBarrier, depthBarrier };
	commandList->ResourceBarrier(3, barriers);
}

void GBuffer::OnResize(int width, int height, ComPtr<ID3D12Device> device) {
	DiffuseTex.Resource.Reset();
	NormalTex.Resource.Reset();
	DepthTex.Resource.Reset();

	CreateTextures(width, height, device);
	CreateRTVandDSV(device);
	CreateSRV(device);
}