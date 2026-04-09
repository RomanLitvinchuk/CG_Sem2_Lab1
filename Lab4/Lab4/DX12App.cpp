#include "DX12App.h"
#include "d3dx12.h"
#include <iostream>
#include <string>
#include <DirectXColors.h>
#include <SimpleMath.h>
#include "d3dUtil.h"
#include "vertex.h"
#include <filesystem>
#include "DDSTextureLoader.h"
#include <numeric>

using namespace DirectX;
using namespace DirectX::SimpleMath;

void DX12App::EnableDebug() {
#if defined(DEBUG) || defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> debugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
	}
#endif
}


void DX12App::InitializeDevice() {
	EnableDebug();
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&m_dxgi_factory_)));

	ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device_)));
	std::cout << "DEVICE CREATED: " << std::endl;

	ThrowIfFailed(m_device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence_)));
	std::cout << "FENCE CREATED" << std::endl;

	m_RTV_descriptor_size_ = m_device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_DSV_descriptor_size_ = m_device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_CbvSrvUav_descriptor_size_ = m_device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	std::cout << "RTV size: " << std::to_string(m_RTV_descriptor_size_) << "\n"
		<< "DSV size: " << std::to_string(m_DSV_descriptor_size_) << "\n"
		<< "CbvSrvUav size:" << std::to_string(m_CbvSrvUav_descriptor_size_) << std::endl;

	msQualityLevels_.Format = m_back_buffer_format_;
	msQualityLevels_.SampleCount = 4;
	msQualityLevels_.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels_.NumQualityLevels = 0;

	ThrowIfFailed(m_device_->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels_, sizeof(msQualityLevels_)));
	if (msQualityLevels_.NumQualityLevels > 0) { std::cout << "MSAA 4x is supported" << std::endl;} 
	else { std::cout << "WARNING! MSAA 4x is NOT supported" << std::endl; }
}

void DX12App::InitializeCommandObjects() {
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(m_device_->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_command_queue_)));
	std::cout << "Command queue is created" << std::endl;
	ThrowIfFailed(m_device_->CreateCommandAllocator(queueDesc.Type, IID_PPV_ARGS(&m_direct_cmd_list_alloc_)));
	std::cout << "Command allocator is created" << std::endl;
	ThrowIfFailed(m_device_->CreateCommandList(0, queueDesc.Type, m_direct_cmd_list_alloc_.Get(), nullptr, IID_PPV_ARGS(&m_command_list_)));
	std::cout << "Command list is created" << std::endl;
	ThrowIfFailed(m_command_list_->Close());
}

void DX12App::CreateSwapChain(HWND hWnd) {
	DXGI_SWAP_CHAIN_DESC swDesc = {};
	swDesc.BufferDesc.Width = m_client_width_;
	swDesc.BufferDesc.Height = m_client_height_;
	swDesc.BufferDesc.RefreshRate.Numerator = 60;
	swDesc.BufferDesc.RefreshRate.Denominator = 1;
	swDesc.BufferDesc.Format = m_back_buffer_format_;
	swDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	//swDesc.SampleDesc.Count = (msQualityLevels_.NumQualityLevels > 0) ? 4 : 1;
	//swDesc.SampleDesc.Quality = (msQualityLevels_.NumQualityLevels > 0) ? msQualityLevels_.NumQualityLevels - 1 : 0;
	swDesc.SampleDesc.Count = 1;
	swDesc.SampleDesc.Quality = 0;
	swDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swDesc.BufferCount = 2;
	swDesc.OutputWindow = hWnd;
	swDesc.Windowed = true;
	swDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	ThrowIfFailed(m_dxgi_factory_->CreateSwapChain(m_command_queue_.Get(), &swDesc, &m_swap_chain_));
	std::cout << "Swap chain is created" << std::endl;
	
}

void DX12App::CreateRTVAndDSVDescriptorHeaps() {
	D3D12_DESCRIPTOR_HEAP_DESC RTVHeapDesc;
	RTVHeapDesc.NumDescriptors = 2;
	RTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	RTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	RTVHeapDesc.NodeMask = 0;
	ThrowIfFailed(m_device_->CreateDescriptorHeap(&RTVHeapDesc, IID_PPV_ARGS(&m_RTV_heap_)));
	std::cout << "RTV heap is created" << std::endl;

	D3D12_DESCRIPTOR_HEAP_DESC DSVHeapDesc;
	DSVHeapDesc.NumDescriptors = 1;
	DSVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	DSVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DSVHeapDesc.NodeMask = 0;
	ThrowIfFailed(m_device_->CreateDescriptorHeap(&DSVHeapDesc, IID_PPV_ARGS(&m_DSV_heap_)));
	std::cout << "DSV heap is created" << std::endl;
}

void DX12App::CreateCBVDescriptorHeap() {
	D3D12_DESCRIPTOR_HEAP_DESC CBV_SRV_HeapDesc;
	CBV_SRV_HeapDesc.NumDescriptors = 1 + mTextures.size();
	CBV_SRV_HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	CBV_SRV_HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	CBV_SRV_HeapDesc.NodeMask = 0;
	ThrowIfFailed(m_device_->CreateDescriptorHeap(&CBV_SRV_HeapDesc, IID_PPV_ARGS(&m_CBV_SRV_heap_)));
	std::cout << "CBV heap is created" << std::endl;
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12App::GetBackBuffer() const {
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_RTV_heap_->GetCPUDescriptorHandleForHeapStart(), m_current_back_buffer_, m_RTV_descriptor_size_);
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12App::GetDSV() const {
	return m_DSV_heap_->GetCPUDescriptorHandleForHeapStart();
}

ID3D12Resource* DX12App::CurrentBackBuffer() const {
	return m_swap_chain_buffer_[m_current_back_buffer_].Get();
}


void DX12App::CreateRTV() {
	CD3DX12_CPU_DESCRIPTOR_HANDLE RTV_heap_handle_(m_RTV_heap_->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < 2; i++) {
		ThrowIfFailed(m_swap_chain_->GetBuffer(i, IID_PPV_ARGS(&m_swap_chain_buffer_[i])));
		m_device_->CreateRenderTargetView(m_swap_chain_buffer_[i].Get(), nullptr, RTV_heap_handle_);
		RTV_heap_handle_.Offset(1, m_RTV_descriptor_size_);
	}
	std::cout << "RTV is created" << std::endl;
}

void DX12App::CreateDSV() {
	D3D12_RESOURCE_DESC dsDesc;
	dsDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	dsDesc.Alignment = 0;
	dsDesc.Width = m_client_width_;
	dsDesc.Height = m_client_height_;
	dsDesc.DepthOrArraySize = 1;
	dsDesc.MipLevels = 1;
	dsDesc.Format = m_depth_stencil_format_;
	dsDesc.SampleDesc.Count = 1;
	dsDesc.SampleDesc.Quality = 0;
	dsDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	dsDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clrValue;
	clrValue.Format = m_depth_stencil_format_;
	clrValue.DepthStencil.Depth = 1.0f;
	clrValue.DepthStencil.Stencil = 0;
	CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(m_device_->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &dsDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clrValue, IID_PPV_ARGS(&m_DSV_buffer)));
	std::cout << "DSV is created" << std::endl;
	m_device_->CreateDepthStencilView(m_DSV_buffer.Get(), nullptr, GetDSV());
}

void DX12App::LoadTextures()
{
	ThrowIfFailed(m_command_list_->Reset(m_direct_cmd_list_alloc_.Get(), nullptr));
	UINT index = 0;

	for (auto& entry : std::filesystem::directory_iterator(L"textures"))
	{
		auto path = entry.path();
		if (path.extension() != L".dds") continue;

		std::wstring name = path.stem().wstring();
		std::transform(name.begin(), name.end(), name.begin(), ::towlower);

		auto tex = std::make_unique<Texture>();
		tex->name_ = std::string(name.begin(), name.end());
		tex->filepath = path.wstring();
		tex->srvHeapIndex = index++;

		ThrowIfFailed(CreateDDSTextureFromFile12(
			m_device_.Get(),
			m_command_list_.Get(),
			tex->filepath.c_str(),
			tex->Resource,
			tex->UploadHeap));

		std::wcout << L"Loaded texture: [" << name << L"] to index: " << tex->srvHeapIndex << std::endl;

		mTextures[name] = std::move(tex);
	}

	ThrowIfFailed(m_command_list_->Close());
	ID3D12CommandList* lists[] = { m_command_list_.Get() };
	m_command_queue_->ExecuteCommandLists(1, lists);
	FlushCommandQueue();
}


void DX12App::CreateSRV() {

	auto handle = m_CBV_SRV_heap_->GetCPUDescriptorHandleForHeapStart();

	for (auto& [name, tex] : mTextures)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE h(handle,
			tex->srvHeapIndex + 1,
			m_CbvSrvUav_descriptor_size_);

		D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		desc.Format = tex->Resource->GetDesc().Format;
		desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipLevels = tex->Resource->GetDesc().MipLevels;

		m_device_->CreateShaderResourceView(tex->Resource.Get(), &desc, h);
		std::cout << "SRV is created" << std::endl;
	}
}

void DX12App::CreateSamplerHeap() {
	D3D12_DESCRIPTOR_HEAP_DESC sampHeapDesc = {};
	sampHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	sampHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	sampHeapDesc.NumDescriptors = 1;
	ThrowIfFailed(m_device_->CreateDescriptorHeap(&sampHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_sampler_heap));
	std::cout << "Sampler heap is created" << std::endl;

	D3D12_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D12_FLOAT32_MAX;
	sampDesc.MipLODBias = 0.0f;
	sampDesc.MaxAnisotropy = 1;
	sampDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	m_device_->CreateSampler(&sampDesc, m_sampler_heap->GetCPUDescriptorHandleForHeapStart());
	std::cout << "Sampler Descriptor is created" << std::endl;
}

void DX12App::SetViewport() {
	vp_.TopLeftX = 0.0f;
	vp_.TopLeftY = 0.0f;
	vp_.Width = static_cast<float>(m_client_width_);
	vp_.Height = static_cast<float>(m_client_height_);
	vp_.MinDepth = 0.0f;
	vp_.MaxDepth = 1.0f;
}

void DX12App::SetScissor() {
	m_scissor_rect_ = { 0, 0, m_client_width_, m_client_height_ };
	std::cout << "Scissor is set" << std::endl;
}

void DX12App::CalculateGameStats(HWND hWnd) {
	static int frameCnt= 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	if ((gt.TotalTime() - timeElapsed) >= 1.0f) {
		float fps = (float)frameCnt;
		float mspf = 1000.0f / fps;
		std::wstring WindowString = L"WINDOW  fps: " + std::to_wstring(fps) + L"mspf: " + std::to_wstring(mspf);
		SetWindowText(hWnd, WindowString.c_str());

		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

void DX12App::FlushCommandQueue()
{
	m_current_fence_++;

	ThrowIfFailed(m_command_queue_->Signal(m_fence_.Get(), m_current_fence_));

	if (m_fence_->GetCompletedValue() < m_current_fence_)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

		ThrowIfFailed(m_fence_->SetEventOnCompletion(m_current_fence_, eventHandle));

		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

void DX12App::DrawToGBuffer(ComPtr<ID3D12GraphicsCommandList> m_command_list_) {
	currentFrameCounter++;
	visibleIndices.clear();

	if (camera.isFrustumCullingEnabled)
	{
		octree.GetVisibleObjects(camera.planes, mSubmeshes, meshVisibilityFences, currentFrameCounter, visibleIndices);
	}
	else
	{
		visibleIndices.resize(mSubmeshes.size());
		std::iota(visibleIndices.begin(), visibleIndices.end(), 0); 
	}

	m_command_list_->SetPipelineState(renderSystem->opaquePSO_.Get());

	m_command_list_->ClearDepthStencilView(GetDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvs[] = {
		renderSystem->g_buffer->DiffuseTex.rtvHandle, renderSystem->g_buffer->NormalTex.rtvHandle
	};
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = renderSystem->g_buffer->DepthTex.dsvHandle;
	m_command_list_->OMSetRenderTargets(2, rtvs, true, &dsv);

	ID3D12DescriptorHeap* descriptorHeaps[] = { m_CBV_SRV_heap_.Get(), m_sampler_heap.Get() };
	m_command_list_->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	m_command_list_->SetGraphicsRootSignature(renderSystem->opaqueRS_.Get());

	m_command_list_->IASetVertexBuffers(0, 1, &VertexBuffers[0]);
	m_command_list_->IASetIndexBuffer(&ibv);
	m_command_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(
		m_CBV_SRV_heap_->GetGPUDescriptorHandleForHeapStart());

	m_command_list_->SetGraphicsRootDescriptorTable(0, cbvHandle);

	m_command_list_->SetGraphicsRootDescriptorTable(
		2,
		m_sampler_heap->GetGPUDescriptorHandleForHeapStart());

	UINT matSize = d3dUtil::CalcConstantBufferSize(sizeof(MaterialConstants));

	m_command_list_->SetGraphicsRootConstantBufferView(6, HullCB->Resource()->GetGPUVirtualAddress());

	for (UINT idx : visibleIndices)
	{
		auto& sm = mSubmeshes[idx];
		UINT currentInstanceOffset = 0;
		for (int i = 0; i < sm.InstanceCount; i++) {
			InstanceBuffer->CopyData(currentInstanceOffset + i, sm.instances[i]);
		}

		m_command_list_->SetGraphicsRootShaderResourceView(7, InstanceBuffer->Resource()->GetGPUVirtualAddress());

		UINT matIndex = sm.materialIndex;
		UINT matSize = d3dUtil::CalcConstantBufferSize(sizeof(MaterialConstants));
		D3D12_GPU_VIRTUAL_ADDRESS matAddress = MaterialCB->Resource()->GetGPUVirtualAddress() + matIndex * matSize;
		m_command_list_->SetGraphicsRootConstantBufferView(3, matAddress);

		int texHeapIndex = materialData[matIndex].diffuseTextureIndex + 1;

		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(
			m_CBV_SRV_heap_->GetGPUDescriptorHandleForHeapStart(),
			texHeapIndex,
			m_CbvSrvUav_descriptor_size_);

		m_command_list_->SetGraphicsRootDescriptorTable(1, srvHandle);

		int normHeapIndex = materialData[matIndex].normalTextureIndex + 1;

		CD3DX12_GPU_DESCRIPTOR_HANDLE normHandle(
			m_CBV_SRV_heap_->GetGPUDescriptorHandleForHeapStart(),
			normHeapIndex,
			m_CbvSrvUav_descriptor_size_);

		int dispHeapIndex = materialData[matIndex].displacementTextureIndex + 1;

		CD3DX12_GPU_DESCRIPTOR_HANDLE dispHandle(
			m_CBV_SRV_heap_->GetGPUDescriptorHandleForHeapStart(),
			dispHeapIndex,
			m_CbvSrvUav_descriptor_size_);
		m_command_list_->SetGraphicsRootDescriptorTable(5, dispHandle);

		m_command_list_->SetGraphicsRootDescriptorTable(4, normHandle);

		if (sm.name_.find("Sketchfab") != std::string::npos)
		{
			m_command_list_->SetPipelineState(renderSystem->bakedPSO_.Get());
			D3D12_VERTEX_BUFFER_VIEW bakedVbv;
			bakedVbv.BufferLocation = SOBuffer_->GetGPUVirtualAddress();
			bakedVbv.StrideInBytes = sizeof(BakedVertex);
			bakedVbv.SizeInBytes = SOMesh.SOVertexCount * sizeof(BakedVertex);

			m_command_list_->IASetVertexBuffers(0, 1, &bakedVbv);
			m_command_list_->DrawInstanced(
				SOMesh.SOVertexCount,
				sm.InstanceCount,
				0,
				currentInstanceOffset);
			m_command_list_->IASetVertexBuffers(0, 1, &VertexBuffers[0]);
		}
		else {
			m_command_list_->DrawIndexedInstanced(
				sm.indexCount,
				sm.InstanceCount,
				sm.startIndiceIndex,
				sm.startVerticeIndex,
				currentInstanceOffset);
		}

		currentInstanceOffset += sm.InstanceCount;

		if (sm.name_.find("Sketchfab") != std::string::npos)
		{
			m_command_list_->SetPipelineState(renderSystem->opaquePSO_.Get());
		}
	}

}

void DX12App::DrawLights(ComPtr<ID3D12GraphicsCommandList> m_command_list_) {
	m_command_list_->SetPipelineState(renderSystem->lightPSO_.Get());
	m_command_list_->SetGraphicsRootSignature(renderSystem->lightRS_.Get());

	for (int i = 0; i < renderSystem->sceneLights_.size(); ++i) {
		LightBuffer->CopyData(i, renderSystem->sceneLights_[i]);
	}

	m_command_list_->ClearRenderTargetView(GetBackBuffer(), Colors::Black, 0, nullptr);

	D3D12_CPU_DESCRIPTOR_HANDLE bb = GetBackBuffer();
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = renderSystem->g_buffer->DepthTex.dsvHandle;
	m_command_list_->OMSetRenderTargets(1, &bb, true, &dsv);
	ID3D12DescriptorHeap* descriptorHeaps[] = { renderSystem->g_buffer->SRVDescriptorHeap.Get(), renderSystem->samplerHeap.Get() };
	m_command_list_->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	m_command_list_->SetGraphicsRootConstantBufferView(0, CameraCB->Resource()->GetGPUVirtualAddress());
	UINT count = (UINT)renderSystem->sceneLights_.size();
	m_command_list_->SetGraphicsRoot32BitConstant(1, count, 0);
	m_command_list_->SetGraphicsRootDescriptorTable(2, renderSystem->g_buffer->SRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	m_command_list_->SetGraphicsRootDescriptorTable(3, renderSystem->samplerHeap->GetGPUDescriptorHandleForHeapStart());

	m_command_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_command_list_->DrawInstanced(3, 1, 0, 0);
}

void DX12App::DrawToStreamOutput(ComPtr<ID3D12GraphicsCommandList> m_command_list_)
{
	m_command_list_->SetPipelineState(renderSystem->streamOutputPSO_.Get());
	m_command_list_->SetGraphicsRootSignature(renderSystem->streamOutputRS_.Get());
	m_command_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_CBV_SRV_heap_.Get(), m_sampler_heap.Get() };
	m_command_list_->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	m_command_list_->IASetVertexBuffers(0, 1, &VertexBuffers[0]);
	m_command_list_->IASetIndexBuffer(&ibv);
	CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(
		m_CBV_SRV_heap_->GetGPUDescriptorHandleForHeapStart());

	m_command_list_->SetGraphicsRootDescriptorTable(0, cbvHandle);

	m_command_list_->SetGraphicsRootDescriptorTable(
		2,
		m_sampler_heap->GetGPUDescriptorHandleForHeapStart());

	m_command_list_->SetGraphicsRootConstantBufferView(6, HullCB->Resource()->GetGPUVirtualAddress());

	for (auto& sm : mSubmeshes) {
		if (sm.name_.find("Sketchfab") != std::string::npos) {
			SOMesh = sm;
			break;
		}
	}

	UINT matIndex = SOMesh.materialIndex;
	UINT matSize = d3dUtil::CalcConstantBufferSize(sizeof(MaterialConstants));
	D3D12_GPU_VIRTUAL_ADDRESS matAddress = MaterialCB->Resource()->GetGPUVirtualAddress() + matIndex * matSize;
	m_command_list_->SetGraphicsRootConstantBufferView(3, matAddress);

	int texHeapIndex = materialData[matIndex].diffuseTextureIndex + 1;

	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(
		m_CBV_SRV_heap_->GetGPUDescriptorHandleForHeapStart(),
		texHeapIndex,
		m_CbvSrvUav_descriptor_size_);

	m_command_list_->SetGraphicsRootDescriptorTable(1, srvHandle);

	int normHeapIndex = materialData[matIndex].normalTextureIndex + 1;

	CD3DX12_GPU_DESCRIPTOR_HANDLE normHandle(
		m_CBV_SRV_heap_->GetGPUDescriptorHandleForHeapStart(),
		normHeapIndex,
		m_CbvSrvUav_descriptor_size_);

	int dispHeapIndex = materialData[matIndex].displacementTextureIndex + 1;

	CD3DX12_GPU_DESCRIPTOR_HANDLE dispHandle(
		m_CBV_SRV_heap_->GetGPUDescriptorHandleForHeapStart(),
		dispHeapIndex,
		m_CbvSrvUav_descriptor_size_);
	m_command_list_->SetGraphicsRootDescriptorTable(5, dispHandle);

	m_command_list_->SetGraphicsRootDescriptorTable(4, normHandle);

	for (int i = 0; i < SOMesh.InstanceCount; i++) {
		InstanceBuffer->CopyData(i, SOMesh.instances[i]);
	}

	m_command_list_->SetGraphicsRootShaderResourceView(7, InstanceBuffer->Resource()->GetGPUVirtualAddress());

	D3D12_STREAM_OUTPUT_BUFFER_VIEW soViews[] = { SOView_ };
	m_command_list_->SOSetTargets(0, 1, soViews);
	m_command_list_->DrawIndexedInstanced(SOMesh.indexCount, 1, SOMesh.startIndiceIndex, SOMesh.startVerticeIndex, 0);
	m_command_list_->SOSetTargets(0, 1, nullptr);
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(filledSizeBuffer_.Get(), D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_command_list_->ResourceBarrier(1, &barrier);
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		SOBuffer_.Get(),
		D3D12_RESOURCE_STATE_STREAM_OUT,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	m_command_list_->ResourceBarrier(1, &barrier);
	m_command_list_->CopyResource(readbackBuffer_.Get(), filledSizeBuffer_.Get());
	m_command_list_->Close();
	ID3D12CommandList* cmdsLists[] = { m_command_list_.Get() };
	m_command_queue_->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	FlushCommandQueue();

	UINT64* mappedData = nullptr;
	readbackBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));
	UINT64 filledBytes = *mappedData;
	readbackBuffer_->Unmap(0, nullptr);

	SOMesh.SOVertexCount = filledBytes / sizeof(BakedVertex);
}

void DX12App::Draw()
{
	ThrowIfFailed(m_direct_cmd_list_alloc_->Reset());
	ThrowIfFailed(m_command_list_->Reset(m_direct_cmd_list_alloc_.Get(), renderSystem->opaquePSO_.Get()));
	m_command_list_->RSSetViewports(1, &vp_);
	m_command_list_->RSSetScissorRects(1, &m_scissor_rect_);

	renderSystem->g_buffer->TransitToOpaqueRenderingState(m_command_list_);

	m_command_list_->ClearRenderTargetView(renderSystem->g_buffer->DiffuseTex.rtvHandle, Color(0.0f, 0.0f, 0.0f, 1.0f), 0, nullptr);
	m_command_list_->ClearRenderTargetView(renderSystem->g_buffer->NormalTex.rtvHandle, Color(0.0f, 0.0f, 0.0f, 1.0f), 0, nullptr);
	m_command_list_->ClearDepthStencilView(renderSystem->g_buffer->DepthTex.dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	if (isFirstFrame) {
		DrawToStreamOutput(m_command_list_);
		ThrowIfFailed(m_command_list_->Reset(m_direct_cmd_list_alloc_.Get(), renderSystem->opaquePSO_.Get()));
		isFirstFrame = false;
	}
	DrawToGBuffer(m_command_list_);
	renderSystem->g_buffer->TransitToLightsRenderingState(m_command_list_);
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_command_list_->ResourceBarrier(1, &barrier);
	DrawLights(m_command_list_);

	m_command_list_->SetPipelineState(renderSystem->bulbPSO_.Get());
	m_command_list_->SetGraphicsRootSignature(renderSystem->bulbRS_.Get());

	m_command_list_->SetGraphicsRootConstantBufferView(0, CBUploadBuffer->Resource()->GetGPUVirtualAddress());

	auto handle = renderSystem->g_buffer->SRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	auto size = m_device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_GPU_DESCRIPTOR_HANDLE lightSrvHandle(handle, 3, size);
	m_command_list_->SetGraphicsRootDescriptorTable(1, lightSrvHandle);

	m_command_list_->IASetVertexBuffers(0, 1, &mSphereVbv);
	m_command_list_->IASetIndexBuffer(&mSphereIbv);

	m_command_list_->DrawIndexedInstanced(mSphereIndexCount, 500, 0, 0, 0);
	CD3DX12_RESOURCE_BARRIER barrierBack = CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);
	m_command_list_->ResourceBarrier(1, &barrierBack);
	ThrowIfFailed(m_command_list_->Close());
	ID3D12CommandList* cmdsLists[] = { m_command_list_.Get() };
	m_command_queue_->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	ThrowIfFailed(m_swap_chain_->Present(0, 0));
	m_current_back_buffer_ = (m_current_back_buffer_ + 1) % 2;

	FlushCommandQueue();
}

void DX12App::InitProjectionMatrix() {
	float aspectRatio = static_cast<float>(m_client_width_) / m_client_height_;

	camera.mProj_ = Matrix::CreatePerspectiveFieldOfView(
		XMConvertToRadians(60.0f),  
		aspectRatio,                
		1.0f,                       
		100000.0f                    
	);

	std::cout << "Projection matrix initialized. Aspect ratio: "
		<< aspectRatio << std::endl;
	
}


void DX12App::CreateVertexBuffer() {
	UINT vbByteSize = (UINT)(vertices.size() * sizeof(Vertex));
	ThrowIfFailed(m_direct_cmd_list_alloc_->Reset());
	ThrowIfFailed(m_command_list_->Reset(m_direct_cmd_list_alloc_.Get(), nullptr));
	VertexBufferGPU_ = d3dUtil::CreateDefaultBuffer(m_device_.Get(), m_command_list_.Get(), vertices.data(), vbByteSize, VertexBufferUploader_);
	ThrowIfFailed(m_command_list_->Close());
	ID3D12CommandList* cmdsLists[] = { m_command_list_.Get() };
	m_command_queue_->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	FlushCommandQueue();
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = VertexBufferGPU_->GetGPUVirtualAddress();
	vbv.SizeInBytes = vbByteSize;
	vbv.StrideInBytes = sizeof(Vertex);
	VertexBuffers[0] = { vbv };
	std::cout << "Vertex Buffer is set" << std::endl;
}


void DX12App::CreateIndexBuffer() {
	UINT ibByteSize = (UINT)(indices.size() * sizeof(std::uint32_t));
	ThrowIfFailed(m_direct_cmd_list_alloc_->Reset());
	ThrowIfFailed(m_command_list_->Reset(m_direct_cmd_list_alloc_.Get(), nullptr));
	IndexBufferGPU_ = d3dUtil::CreateDefaultBuffer(m_device_.Get(), m_command_list_.Get(), indices.data(), ibByteSize, IndexBufferUploader_);
	ThrowIfFailed(m_command_list_->Close());
	ID3D12CommandList* cmdsLists[] = { m_command_list_.Get() };
	m_command_queue_->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	FlushCommandQueue();
	ibv.BufferLocation = IndexBufferGPU_->GetGPUVirtualAddress();
	ibv.SizeInBytes = ibByteSize;
	ibv.Format = DXGI_FORMAT_R32_UINT;
	std::cout << "Index buffer is set" << std::endl;
}


void DX12App::OnMouseDown(HWND hWnd) {
	SetCapture(hWnd);
}

void DX12App::OnMouseUp() {
	ReleaseCapture();
}

void DX12App::Update() {
	camera.UpdateCameraPos(m_key_states, gt);

	camera.mView_ = Matrix::CreateLookAt(camera.mCameraPos, camera.mCameraPos + camera.mCameraTarget, camera.mCameraUp);

	std::cout << "CameraPos:" << camera.mCameraPos.x << " " << camera.mCameraPos.y << " " << camera.mCameraPos.z << std::endl;
	Matrix ViewProj = camera.mView_ * camera.mProj_;

	if (camera.isFrustumCullingEnabled){
		XMMATRIX M = ViewProj;
		XMFLOAT4X4 m;
		XMStoreFloat4x4(&m, M);

		camera.planes[0] = XMVectorSet(m._14 + m._11, m._24 + m._21, m._34 + m._31, m._44 + m._41);
		camera.planes[1] = XMVectorSet(m._14 - m._11, m._24 - m._21, m._34 - m._31, m._44 - m._41);
		camera.planes[2] = XMVectorSet(m._14 + m._12, m._24 + m._22, m._34 + m._32, m._44 + m._42);
		camera.planes[3] = XMVectorSet(m._14 - m._12, m._24 - m._22, m._34 - m._32, m._44 - m._42);
		camera.planes[4] = XMVectorSet(m._13, m._23, m._33, m._43);
		camera.planes[5] = XMVectorSet(m._14 - m._13, m._24 - m._23, m._34 - m._33, m._44 - m._43);

		for (int i = 0; i < 6; ++i) camera.planes[i] = XMPlaneNormalize(camera.planes[i]);
	}

	Matrix InvViewProj = ViewProj.Invert();
	ViewProj = ViewProj.Transpose();
	InvViewProj = InvViewProj.Transpose();

	ObjectConstants obj;
	Matrix TWorld = camera.mWorld_.Transpose();
	obj.mViewProj = ViewProj;
	obj.gTime = gt.TotalTime();
	CBUploadBuffer->CopyData(0, obj);

	for (int i = 0; i < materialData.size(); ++i) {
		MaterialCB->CopyData(i, materialData[i]);
	}

	CameraConstants camConst;
	camConst.invViewProj = InvViewProj;
	camConst.cameraPos = camera.mCameraPos;
	CameraCB->CopyData(0, camConst);

	HullBuffer HullConst;
	HullConst.CameraPos = camera.mCameraPos;
	HullConst.gMinTess = 1;
	HullConst.gMaxTess = 5;
	HullConst.gMinDist = 10.0f;
	HullConst.gMaxDist = 200.0f;
	HullCB->CopyData(0, HullConst);

	for (int i = 0; i < materialData.size(); ++i) {
		//materialData[i].MatTransform = Matrix::Identity;
		if (materialData[i].isTree == 1) {
			float tu = materialData[i].MatTransform(1, 0);
			float tv = materialData[i].MatTransform(1, 1);
			tu += 0.1f * gt.DeltaTime();
			tv += 0.02f * gt.DeltaTime();

			if (tu >= 1.0f)
				tu -= 1.0f;
			if (tv >= 1.0f)
				tv -= 1.0f;
			materialData[i].MatTransform(1, 0) = tu;
			materialData[i].MatTransform(1, 1) = tv;
		}
		MaterialCB->CopyData(i, materialData[i]);
	}

	static float timeAccumulator = 0.0f;
	timeAccumulator += gt.DeltaTime();

	if (timeAccumulator >= 1.0f) {
		timeAccumulator = 0.0f;

		for (int i = 0; i < renderSystem->sceneLights_.size(); i++) {
			if (renderSystem->sceneLights_[i].lightType == 1) {
				renderSystem->sceneLights_[i].lightColor.x = static_cast<float>(rand()) / RAND_MAX;
				renderSystem->sceneLights_[i].lightColor.y = static_cast<float>(rand()) / RAND_MAX;
				renderSystem->sceneLights_[i].lightColor.z = static_cast<float>(rand()) / RAND_MAX;
			}
			LightBuffer->CopyData(i, renderSystem->sceneLights_[i]);
		}
	}
}

void DX12App::InitUploadBuffers() {
	CBUploadBuffer = std::make_unique<UploadBuffer<ObjectConstants>>(
		m_device_.Get(),
		1,
		true
	);
	MaterialCB = std::make_unique<UploadBuffer<MaterialConstants>>(m_device_.Get(), 300, true);
	CameraCB = std::make_unique<UploadBuffer<CameraConstants>>(m_device_.Get(), 1, true);
	LightBuffer = std::make_unique<UploadBuffer<LightConstants>>(m_device_.Get(), 1000, false);
	InstanceBuffer = std::make_unique<UploadBuffer<InstanceData>>(m_device_.Get(), 1000, false);
	HullCB = std::make_unique<UploadBuffer<HullBuffer>>(m_device_.Get(), 1, true);
}

void DX12App::CreateConstantBufferView() {
	UINT cbByteSize = d3dUtil::CalcConstantBufferSize(sizeof(ObjectConstants));
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = CBUploadBuffer->Resource()->GetGPUVirtualAddress();
	int BoxCBIndex = 0;
	cbAddress += BoxCBIndex * cbByteSize;
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbDesc;
	cbDesc.BufferLocation = cbAddress;
	cbDesc.SizeInBytes = cbByteSize;
	m_device_->CreateConstantBufferView(&cbDesc, m_CBV_SRV_heap_->GetCPUDescriptorHandleForHeapStart());
	std::cout << "Constant buffer view is created" << std::endl;
}

void DX12App::BuildLayout() {
	m_input_layout_ =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};
	
	bakedLayout_ = {
		{ "WORLDPOS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void DX12App::OnResize() {
	FlushCommandQueue();
	ThrowIfFailed(m_command_list_->Reset(m_direct_cmd_list_alloc_.Get(), nullptr));
	m_swap_chain_buffer_[0].Reset();
	m_swap_chain_buffer_[1].Reset();

	ThrowIfFailed(m_swap_chain_->ResizeBuffers(2, m_client_width_, m_client_height_, m_back_buffer_format_, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	InitProjectionMatrix();
	m_current_back_buffer_ = 0;
	CreateRTV();
	CreateDSV();
	renderSystem->g_buffer->OnResize(m_client_width_, m_client_height_, m_device_);
	SetViewport();
	SetScissor();
	ThrowIfFailed(m_command_list_->Close());
	ID3D12CommandList* cmdsLists[] = { m_command_list_.Get() };
	m_command_queue_->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	FlushCommandQueue();
}

void DX12App::InitRenderSystem() {
	renderSystem = new RenderingSystem(m_device_, m_input_layout_, bakedLayout_, m_client_width_, m_client_height_);

	CreateStructuredBuffersSRV();
}


void DX12App::Parsing() {
	ParseFile("models/sponza.obj", Matrix::Identity, 1);

	Matrix Transform = Matrix::CreateScale(0.2f) * Matrix::CreateRotationX(-3.14 / 2) * Matrix::CreateTranslation(0.0f, 0.0f, 0.0f);
	ParseFile("models/Christmas Tree Color mm.obj", Transform, 1);

	Transform = Matrix::CreateScale(25.0f) * Matrix::CreateTranslation(100.0f, 500.0f, 0.0f);
	ParseFile("models/Sketchfab.fbx", Transform, 1);

	Transform = Matrix::CreateScale(25.0f) * Matrix::CreateTranslation(400.0f, 200.0f, 0.0f);
	ParseFile("models/TOPOR.obj", Transform, 1);

	Transform = Matrix::CreateScale(100.0f) * Matrix::CreateRotationX(3.14 / 2) * Matrix::CreateTranslation(400.0f, 110.0f, -50.0f);
	ParseFile("models/SM_Chisel.fbx", Transform, 500);


	meshVisibilityFences.assign(mSubmeshes.size(), 0);
	visibleIndices.reserve(mSubmeshes.size());

	octree.Build(mSubmeshes, 5, 10);
}
 
void DX12App::CreateStructuredBuffersSRV() {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.NumElements = 1000;
	srvDesc.Buffer.StructureByteStride = sizeof(LightConstants);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	auto handle = renderSystem->g_buffer->SRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	auto size = m_device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE SrvHandle(handle, 3, size);

	m_device_->CreateShaderResourceView(LightBuffer->Resource(), &srvDesc, SrvHandle);
}

void DX12App::CreateSOBuffers() {
	UINT maxTess = 3;
	UINT maxVerticesPerPatch = (maxTess * maxTess) * 3;
	UINT totalMaxVertices = 583680 * maxVerticesPerPatch;
	UINT64 soBufferSize = totalMaxVertices * sizeof(BakedVertex);

	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto SOBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(soBufferSize, D3D12_RESOURCE_FLAG_NONE);
	ThrowIfFailed(m_device_->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, 
		&SOBufferDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&SOBuffer_)));

	SOBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(8, D3D12_RESOURCE_FLAG_NONE);
	ThrowIfFailed(m_device_->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE,
		&SOBufferDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&filledSizeBuffer_)));

	heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
	ThrowIfFailed(m_device_->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE,
		&SOBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readbackBuffer_)));

	CD3DX12_RESOURCE_BARRIER soBarrier = CD3DX12_RESOURCE_BARRIER::Transition(SOBuffer_.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_STREAM_OUT);
	CD3DX12_RESOURCE_BARRIER fsBarrier = CD3DX12_RESOURCE_BARRIER::Transition(filledSizeBuffer_.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_STREAM_OUT);
	D3D12_RESOURCE_BARRIER barriers[] = { soBarrier, fsBarrier };
	m_command_list_->Reset(m_direct_cmd_list_alloc_.Get(), nullptr);
	m_command_list_->ResourceBarrier(2, barriers);
	m_command_list_->Close();
	ID3D12CommandList* cmdsLists[] = { m_command_list_.Get() };
	m_command_queue_->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	FlushCommandQueue();

	SOView_.BufferLocation = SOBuffer_->GetGPUVirtualAddress();
	SOView_.SizeInBytes = soBufferSize;
	SOView_.BufferFilledSizeLocation = filledSizeBuffer_->GetGPUVirtualAddress();

}