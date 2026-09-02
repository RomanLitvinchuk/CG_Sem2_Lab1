#include "DX12App.h"

void SSAO::CreateTexture(ComPtr<ID3D12Device> device, int width, int height) {
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_FLOAT, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_CLEAR_VALUE clearValue;
	clearValue.Format = DXGI_FORMAT_R32_FLOAT;
	clearValue.Color[0] = 0.0f;
	clearValue.Color[1] = 0.0f;
	clearValue.Color[2] = 0.0f;
	clearValue.Color[3] = 1.0f;
	device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue, IID_PPV_ARGS(&SSAOTexture_A.Resource));
	SSAOTexture_A.Resource->SetName(L"SSAO Texture A");
	SSAOTexture_A.currentState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	
	device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearValue, IID_PPV_ARGS(&SSAOTexture_B.Resource));
	SSAOTexture_B.Resource->SetName(L"SSAO Texture B");
	SSAOTexture_B.currentState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
}

void SSAO::CreateRTV(ComPtr<ID3D12Device> device) {
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(rtvHeap_->GetCPUDescriptorHandleForHeapStart());
	auto rtvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;
	SSAOTexture_A.rtvHandle = handle;
	device->CreateRenderTargetView(SSAOTexture_A.Resource.Get(), &rtvDesc, handle);

	handle.Offset(1, rtvSize);
	SSAOTexture_B.rtvHandle = handle;
	device->CreateRenderTargetView(SSAOTexture_B.Resource.Get(), &rtvDesc, handle);
}

void SSAO::CreateSRV(ComPtr<ID3D12Device> device, ID3D12Resource* depthTexture, ID3D12Resource* normalTexture, ID3D12Resource* noiseTexture) {
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(srvHeap_->GetCPUDescriptorHandleForHeapStart());
	auto size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;
	SSAOTexture_A.srvHandle = handle;
	device->CreateShaderResourceView(SSAOTexture_A.Resource.Get(), &srvDesc, handle);
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(srvHeap_->GetGPUDescriptorHandleForHeapStart());
	SSAOTexture_A.srvGpuHandle = gpuHandle;

	handle.Offset(1, size);
	gpuHandle.Offset(1, size);
	SSAOTexture_B.srvHandle = handle;
	SSAOTexture_B.srvGpuHandle = gpuHandle;
	device->CreateShaderResourceView(SSAOTexture_B.Resource.Get(), &srvDesc, handle);

	handle.Offset(1, size);
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	device->CreateShaderResourceView(depthTexture, &srvDesc, handle);

	handle.Offset(1, size);
	srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	device->CreateShaderResourceView(normalTexture, &srvDesc, handle);

	handle.Offset(1, size);
	D3D12_RESOURCE_DESC noiseDesc = noiseTexture->GetDesc();
	srvDesc.Format = noiseDesc.Format;
	device->CreateShaderResourceView(noiseTexture, &srvDesc, handle);
}

void SSAO::CreateSamplers(ComPtr<ID3D12Device> device)
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE sampHandle(samplerHeap_->GetCPUDescriptorHandleForHeapStart());
	auto sampSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	D3D12_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	sampDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D12_FLOAT32_MAX;
	sampDesc.MipLODBias = 0.0f;
	sampDesc.MaxAnisotropy = 1;
	sampDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	device->CreateSampler(&sampDesc, sampHandle);

	sampHandle.Offset(1, sampSize);
	sampDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	device->CreateSampler(&sampDesc, sampHandle);
}

void SSAO::OnResize(ComPtr<ID3D12Device> device, int width, int height, ID3D12Resource* depthTexture, ID3D12Resource* normalTexture, ID3D12Resource* noiseTexture)
{
	SSAOTexture_A.Resource.Reset();
	CreateTexture(device, width, height);
	CreateRTV(device);
	CreateSRV(device, depthTexture, normalTexture, noiseTexture);
}

void SSAO::ClearSSAO(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	d3dUtil::Barrier(commandList, &SSAOTexture_A, D3D12_RESOURCE_STATE_RENDER_TARGET);
	d3dUtil::Barrier(commandList, &SSAOTexture_B, D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList->ClearRenderTargetView(SSAOTexture_A.rtvHandle, Color(0.0f, 0.0f, 0.0f, 1.0f), 0, nullptr);
	commandList->ClearRenderTargetView(SSAOTexture_B.rtvHandle, Color(0.0f, 0.0f, 0.0f, 1.0f), 0, nullptr);
	BarriersToDefault(commandList);
}

void SSAO::BarriersToDefault(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	d3dUtil::Barrier(commandList, &SSAOTexture_A, D3D12_RESOURCE_STATE_RENDER_TARGET);
	d3dUtil::Barrier(commandList, &SSAOTexture_B, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void DX12App::DrawSSAO() {
	m_command_list_->SetPipelineState(renderSystem->SsaoPSO_.Get());
	m_command_list_->SetGraphicsRootSignature(renderSystem->SsaoRS_.Get());

	D3D12_VIEWPORT ssaoViewport = {};
	ssaoViewport.TopLeftX = 0.0f;
	ssaoViewport.TopLeftY = 0.0f;
	ssaoViewport.Width = static_cast<float>(m_client_width_ / 2);
	ssaoViewport.Height = static_cast<float>(m_client_height_ / 2);
	ssaoViewport.MinDepth = 0.0f;
	ssaoViewport.MaxDepth = 1.0f;

	D3D12_RECT ssaoScissorRect = { 0, 0, m_client_width_ / 2, m_client_height_ / 2 };

	m_command_list_->RSSetViewports(1, &ssaoViewport);
	m_command_list_->RSSetScissorRects(1, &ssaoScissorRect);

	auto handle = renderSystem->ssao->SSAOTexture_A.rtvHandle;
	m_command_list_->OMSetRenderTargets(1, &handle, true, nullptr);

	ID3D12DescriptorHeap* descriptorHeaps[] = { renderSystem->ssao->srvHeap_.Get(), renderSystem->ssao->samplerHeap_.Get()};
	m_command_list_->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(renderSystem->ssao->srvHeap_->GetGPUDescriptorHandleForHeapStart());
	auto srvSize = m_device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	srvHandle.Offset(2, srvSize);
	m_command_list_->SetGraphicsRootDescriptorTable(0, srvHandle);

	m_command_list_->SetGraphicsRootDescriptorTable(1, renderSystem->ssao->samplerHeap_->GetGPUDescriptorHandleForHeapStart());

	m_command_list_->SetGraphicsRootConstantBufferView(2, SsaoBuffer->Resource()->GetGPUVirtualAddress());
	m_command_list_->SetGraphicsRootConstantBufferView(3, CameraCB->Resource()->GetGPUVirtualAddress());
	m_command_list_->SetGraphicsRootConstantBufferView(4, MatricesBuffer->Resource()->GetGPUVirtualAddress());

	m_command_list_->DrawInstanced(3, 1, 0, 0);

	//m_command_list_->RSSetViewports(1, &vp_);
	//m_command_list_->RSSetScissorRects(1, &m_scissor_rect_);
}

void DX12App::BlurSSAO()
{
	MyTexture* ssaoWriteTexture = &renderSystem->ssao->SSAOTexture_A;
	MyTexture* ssaoReadTexture = &renderSystem->ssao->SSAOTexture_B;

	d3dUtil::SwapTextures(m_command_list_, ssaoWriteTexture, ssaoReadTexture);

	m_command_list_->SetPipelineState(renderSystem->SsaoBlurPSO_.Get());
	m_command_list_->SetGraphicsRootSignature(renderSystem->SsaoBlurRS_.Get());

	auto rtv = ssaoWriteTexture->rtvHandle;
	m_command_list_->OMSetRenderTargets(1, &rtv, true, nullptr);

	m_command_list_->SetGraphicsRootDescriptorTable(0, ssaoReadTexture->srvGpuHandle);

	CD3DX12_GPU_DESCRIPTOR_HANDLE normalHandle(renderSystem->ssao->srvHeap_->GetGPUDescriptorHandleForHeapStart());
	auto size = m_device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	normalHandle.Offset(3, size);
	m_command_list_->SetGraphicsRootDescriptorTable(1, normalHandle);

	m_command_list_->SetGraphicsRootDescriptorTable(2, renderSystem->ssao->samplerHeap_->GetGPUDescriptorHandleForHeapStart());

	BlurConstants blurConst;
	blurConst.screenWidth = m_client_width_ / 2;
	blurConst.screenHeight = m_client_height_ / 2;
	blurConst.blurType = 0.0f;
	blurConst.padding = 0.0f;
	SsaoBlurBuffer->CopyData(0, blurConst);

	m_command_list_->SetGraphicsRootConstantBufferView(3, SsaoBlurBuffer->Resource()->GetGPUVirtualAddress());

	m_command_list_->DrawInstanced(3, 1, 0, 0);

	d3dUtil::SwapTextures(m_command_list_, ssaoWriteTexture, ssaoReadTexture);
	rtv = ssaoWriteTexture->rtvHandle;
	m_command_list_->OMSetRenderTargets(1, &rtv, true, nullptr);
	m_command_list_->SetGraphicsRootDescriptorTable(0, ssaoReadTexture->srvGpuHandle);

	blurConst.blurType = 1.0f;
	SsaoBlurBuffer->CopyData(0, blurConst);
	m_command_list_->SetGraphicsRootConstantBufferView(3, SsaoBlurBuffer->Resource()->GetGPUVirtualAddress());

	m_command_list_->DrawInstanced(3, 1, 0, 0);

	m_command_list_->RSSetViewports(1, &vp_);
	m_command_list_->RSSetScissorRects(1, &m_scissor_rect_);
}
