#include "post_process.h"
#include "d3dUtil.h"

void PostProcess::CreateTexture(int width, int height, ComPtr<ID3D12Device> device) {
	D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_CLEAR_VALUE clearValue;
	clearValue.Color[0] = 0.0f;
	clearValue.Color[1] = 0.0f;
	clearValue.Color[2] = 0.0f;
	clearValue.Color[3] = 1.0f;
	clearValue.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue, IID_PPV_ARGS(&HDR_Texture_A.Resource)));
	HDR_Texture_A.Resource->SetName(L"HDR Post Process Texture A");
	HDR_Texture_A.currentState = D3D12_RESOURCE_STATE_RENDER_TARGET;

	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearValue, IID_PPV_ARGS(&HDR_Texture_B.Resource)));
	HDR_Texture_B.Resource->SetName(L"HDR Post Process Texture B");
	HDR_Texture_B.currentState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue, IID_PPV_ARGS(&LDR_Texture_A.Resource)));
	LDR_Texture_A.Resource->SetName(L"LDR Post Process Texture A");
	LDR_Texture_A.currentState = D3D12_RESOURCE_STATE_RENDER_TARGET;

	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearValue, IID_PPV_ARGS(&LDR_Texture_B.Resource)));
	LDR_Texture_B.Resource->SetName(L"LDR Post Process Texture B");
	LDR_Texture_B.currentState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
} 

void PostProcess::CreateSRV(ComPtr<ID3D12Device> device) {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	auto srvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(srvHeap_->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(srvHeap_->GetGPUDescriptorHandleForHeapStart());
	HDR_Texture_A.srvHandle = cpuHandle;
	HDR_Texture_A.srvGpuHandle = gpuHandle;
	device->CreateShaderResourceView(HDR_Texture_A.Resource.Get(), &srvDesc, cpuHandle);
	cpuHandle.Offset(1, srvSize);
	gpuHandle.Offset(1, srvSize);
	HDR_Texture_B.srvHandle = cpuHandle;
	HDR_Texture_B.srvGpuHandle = gpuHandle;
	device->CreateShaderResourceView(HDR_Texture_B.Resource.Get(), &srvDesc, cpuHandle);
	
	cpuHandle.Offset(1, srvSize);
	gpuHandle.Offset(1, srvSize);
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	LDR_Texture_A.srvHandle = cpuHandle;
	LDR_Texture_A.srvGpuHandle = gpuHandle;
	device->CreateShaderResourceView(LDR_Texture_A.Resource.Get(), &srvDesc, cpuHandle);
	cpuHandle.Offset(1, srvSize);
	gpuHandle.Offset(1, srvSize);
	LDR_Texture_B.srvHandle = cpuHandle;
	LDR_Texture_B.srvGpuHandle = gpuHandle;
	device->CreateShaderResourceView(LDR_Texture_B.Resource.Get(), &srvDesc, cpuHandle);
}

void PostProcess::CreateRTV(ComPtr<ID3D12Device> device)
{
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap_->GetCPUDescriptorHandleForHeapStart());
	auto rtvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	HDR_Texture_A.rtvHandle = rtvHandle;
	device->CreateRenderTargetView(HDR_Texture_A.Resource.Get(), &rtvDesc, rtvHandle);
	rtvHandle.Offset(1, rtvSize);
	HDR_Texture_B.rtvHandle = rtvHandle;
	device->CreateRenderTargetView(HDR_Texture_B.Resource.Get(), &rtvDesc, rtvHandle);

	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	rtvHandle.Offset(1, rtvSize);
	LDR_Texture_A.rtvHandle = rtvHandle;
	device->CreateRenderTargetView(LDR_Texture_A.Resource.Get(), &rtvDesc, rtvHandle);
	rtvHandle.Offset(1, rtvSize);
	LDR_Texture_B.rtvHandle = rtvHandle;
	device->CreateRenderTargetView(LDR_Texture_B.Resource.Get(), &rtvDesc, rtvHandle);

}

void PostProcess::ClearPostProcess(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	d3dUtil::Barrier(commandList, &HDR_Texture_A, D3D12_RESOURCE_STATE_RENDER_TARGET);
	d3dUtil::Barrier(commandList, &HDR_Texture_B, D3D12_RESOURCE_STATE_RENDER_TARGET);
	d3dUtil::Barrier(commandList, &LDR_Texture_A, D3D12_RESOURCE_STATE_RENDER_TARGET);
	d3dUtil::Barrier(commandList, &LDR_Texture_B, D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList->ClearRenderTargetView(HDR_Texture_A.rtvHandle, Color(0.0f, 0.0f, 0.0f, 1.0f), 0, nullptr);
	commandList->ClearRenderTargetView(HDR_Texture_B.rtvHandle, Color(0.0f, 0.0f, 0.0f, 1.0f), 0, nullptr);
	commandList->ClearRenderTargetView(LDR_Texture_A.rtvHandle, Color(0.0f, 0.0f, 0.0f, 1.0f), 0, nullptr);
	commandList->ClearRenderTargetView(LDR_Texture_B.rtvHandle, Color(0.0f, 0.0f, 0.0f, 1.0f), 0, nullptr);
	BarriersToDefault(commandList);
}

void PostProcess::BarriersToDefault(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	d3dUtil::Barrier(commandList, &HDR_Texture_A, D3D12_RESOURCE_STATE_RENDER_TARGET);
	d3dUtil::Barrier(commandList, &HDR_Texture_B, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	d3dUtil::Barrier(commandList, &LDR_Texture_A, D3D12_RESOURCE_STATE_RENDER_TARGET);
	d3dUtil::Barrier(commandList, &LDR_Texture_B, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void PostProcess::OnResize(int width, int height, ComPtr<ID3D12Device> device)
{
	HDR_Texture_A.Resource.Reset();

	CreateTexture(width, height, device);
	CreateSRV(device);
	CreateRTV(device);
}
