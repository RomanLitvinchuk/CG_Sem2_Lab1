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
	std::cout << "Command allocatoris is created" << std::endl;
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

void DX12App::CalculateGameStats(GameTimer& gt, HWND hWnd) {
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
	m_command_list_->SetPipelineState(renderSystem->opaquePSO_.Get());

	m_command_list_->ClearDepthStencilView(GetDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	//D3D12_CPU_DESCRIPTOR_HANDLE bb = GetBackBuffer();
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

	for (auto& sm : mSubmeshes)
	{
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

		m_command_list_->DrawIndexedInstanced(
			sm.indexCount,
			1,
			sm.startIndiceIndex,
			sm.startVerticeIndex,
			0);
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

void DX12App::Draw(const GameTimer& gt)
{
	ThrowIfFailed(m_direct_cmd_list_alloc_->Reset());
	ThrowIfFailed(m_command_list_->Reset(m_direct_cmd_list_alloc_.Get(), renderSystem->opaquePSO_.Get()));
	m_command_list_->RSSetViewports(1, &vp_);
	m_command_list_->RSSetScissorRects(1, &m_scissor_rect_);

	renderSystem->g_buffer->TransitToOpaqueRenderingState(m_command_list_);

	m_command_list_->ClearRenderTargetView(renderSystem->g_buffer->DiffuseTex.rtvHandle, Color(0.0f, 0.0f, 0.0f, 1.0f), 0, nullptr);
	m_command_list_->ClearRenderTargetView(renderSystem->g_buffer->NormalTex.rtvHandle, Color(0.0f, 0.0f, 0.0f, 1.0f), 0, nullptr);
	m_command_list_->ClearDepthStencilView(renderSystem->g_buffer->DepthTex.dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

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

	mProj_ = Matrix::CreatePerspectiveFieldOfView(
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


void DX12App::OnMouseMove(WPARAM btnState, int dx, int dy) {
	if ((btnState & MK_LBUTTON) != 0)
	{
		mCameraYaw += static_cast<float>(dx) * mCameraRotationSpeed * 0.01f * -1.0f;
		mCameraPitch -= static_cast<float>(dy) * mCameraRotationSpeed * 0.01f * -1.0f;

		const float limit = XM_PIDIV2 - 0.01f;
		mCameraPitch = std::clamp(mCameraPitch, -limit, limit);

		mCameraTarget.x = cos(mCameraPitch) * sin(mCameraYaw);
		mCameraTarget.y = sin(mCameraPitch);
		mCameraTarget.z = cos(mCameraPitch) * cos(mCameraYaw);
		mCameraTarget.Normalize();
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		mRadius_ += 0.005f * static_cast<float>(dx - dy);

		mRadius_ = mRadius_ < 3.0f ? 3.0f : (mRadius_ > 15.0f ? 15.0f : mRadius_);
	}
}

void DX12App::Update(const GameTimer& gt) {
	float dt = gt.DeltaTime();
	if (GetAsyncKeyState('W') & 0x8000) mCameraPos += mCameraTarget * mCameraSpeed * dt;
	if (GetAsyncKeyState('S') & 0x8000) mCameraPos -= mCameraTarget * mCameraSpeed * dt;
	if (GetAsyncKeyState('A') & 0x8000) mCameraPos -= mCameraTarget.Cross(mCameraUp) * mCameraSpeed * dt;
	if (GetAsyncKeyState('D') & 0x8000) mCameraPos += mCameraTarget.Cross(mCameraUp) * mCameraSpeed * dt;

	Vector3 targetPos = mCameraPos + mCameraTarget;
	if (mCameraPos == targetPos) {
		targetPos += Vector3(0.001f, 0.0f, 0.0f);
	}

	mView_ = Matrix::CreateLookAt(mCameraPos, mCameraPos + mCameraTarget, mCameraUp);

	std::cout << "CameraPos:" << mCameraPos.x << " " << mCameraPos.y << " " << mCameraPos.z << std::endl;
	Matrix ViewProj = mView_ * mProj_;
	Matrix InvViewProj = ViewProj.Invert();
	ViewProj = ViewProj.Transpose();
	InvViewProj = InvViewProj.Transpose();

	ObjectConstants obj;
	obj.mWorld = mWorld_;
	obj.mViewProj = ViewProj;
	obj.mTexTransform = Matrix::Identity;
	obj.gTime = gt.TotalTime();
	CBUploadBuffer->CopyData(0, obj);

	for (int i = 0; i < materialData.size(); ++i) {
		MaterialCB->CopyData(i, materialData[i]);
	}

	CameraConstants camConst;
	camConst.invViewProj = InvViewProj;
	camConst.cameraPos = mCameraPos;
	CameraCB->CopyData(0, camConst);

	for (int i = 0; i < materialData.size(); ++i) {
		//materialData[i].MatTransform = Matrix::Identity;
		if (materialData[i].isTree != 1) {
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
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};
	std::cout << "Layout is builded" << std::endl;
}

void DX12App::OnResize() {
	FlushCommandQueue();
	ThrowIfFailed(m_command_list_->Reset(m_direct_cmd_list_alloc_.Get(), nullptr));
	m_swap_chain_buffer_[0].Reset();
	m_swap_chain_buffer_[1].Reset();

	ThrowIfFailed(m_swap_chain_->ResizeBuffers(2, m_client_width_, m_client_height_, m_back_buffer_format_, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	m_current_back_buffer_ = 0;
	CreateRTV();
	CreateDSV();
	renderSystem->g_buffer->OnResize(m_client_width_, m_client_height_, m_device_);
	SetViewport();
}

void DX12App::InitRenderSystem() {
	renderSystem = new RenderingSystem(m_device_, m_input_layout_, m_client_width_, m_client_height_);

	CreateLightBufferSRV();
}


void DX12App::Parsing() {
	ParseFile("models/sponza.obj", Matrix::Identity);

	Matrix treeTransform = Matrix::CreateScale(0.2f) * Matrix::CreateRotationX(-3.14 / 2) * Matrix::CreateTranslation(0.0f, 0.0f, 0.0f);
	ParseFile("models/Christmas Tree Color mm.obj", treeTransform);
}

void DX12App::CreateLightBufferSRV() {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.NumElements = 1000;
	srvDesc.Buffer.StructureByteStride = sizeof(LightConstants);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	auto handle = renderSystem->g_buffer->SRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	auto size = m_device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE lightSrvHandle(handle, 3, size);

	m_device_->CreateShaderResourceView(LightBuffer->Resource(), &srvDesc, lightSrvHandle);

	std::cout << "StructuredBuffer SRV created in G-Buffer heap at slot 3" << std::endl;
}