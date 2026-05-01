#include "DX12App.h"
#include "d3dx12.h"
#include <iostream>
#include <string>
#include <DirectXColors.h>
#include <SimpleMath.h>
#include "d3dUtil.h"
#include "vertex.h"
#include <filesystem>
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
	DSVHeapDesc.NumDescriptors = 2;
	DSVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	DSVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DSVHeapDesc.NodeMask = 0;
	ThrowIfFailed(m_device_->CreateDescriptorHeap(&DSVHeapDesc, IID_PPV_ARGS(&m_DSV_heap_)));
	std::cout << "DSV heap is created" << std::endl;
}

void DX12App::CreateCBVDescriptorHeap() {
	D3D12_DESCRIPTOR_HEAP_DESC CBV_SRV_HeapDesc;
	CBV_SRV_HeapDesc.NumDescriptors = 2 + mTextures.size();
	CBV_SRV_HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	CBV_SRV_HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	CBV_SRV_HeapDesc.NodeMask = 0;
	ThrowIfFailed(m_device_->CreateDescriptorHeap(&CBV_SRV_HeapDesc, IID_PPV_ARGS(&m_CBV_SRV_heap_)));
	
	D3D12_DESCRIPTOR_HEAP_DESC UAV_heapDesc;
	UAV_heapDesc.NumDescriptors = 10;
	UAV_heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	UAV_heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	UAV_heapDesc.NodeMask = 0;
	ThrowIfFailed(m_device_->CreateDescriptorHeap(&UAV_heapDesc, IID_PPV_ARGS(&UAVHeap_)));
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

void DX12App::InitUploadBuffers() {
	CBUploadBuffer = std::make_unique<UploadBuffer<ObjectConstants>>(
		m_device_.Get(),
		1,
		true
	);
	MatricesBuffer = std::make_unique<UploadBuffer<Matrices>>(m_device_.Get(), 1, true);
	ParticleConstantsBuffer = std::make_unique<UploadBuffer<ParticleConstants>>(m_device_.Get(), 1, true);
	MaterialCB = std::make_unique<UploadBuffer<MaterialConstants>>(m_device_.Get(), 300, true);
	CameraCB = std::make_unique<UploadBuffer<CameraConstants>>(m_device_.Get(), 1, true);
	LightBuffer = std::make_unique<UploadBuffer<LightConstants>>(m_device_.Get(), 1000, false);
	InstanceBuffer = std::make_unique<UploadBuffer<MeshInstanceData>>(m_device_.Get(), 1000, false);
	HullCB = std::make_unique<UploadBuffer<HullBuffer>>(m_device_.Get(), 1, true);
	WireframeInstanceBuffer = std::make_unique<UploadBuffer<WireframeInstanceData>>(m_device_.Get(), 1000, false); 
	ShadowCB = std::make_unique<UploadBuffer<ShadowConstants>>(m_device_.Get(), 1, true);

	DeadListUpload_ = std::make_unique<UploadBuffer<uint32_t>>(m_device_.Get(), PARTICLE_COUNT, false);
	deadCounterUpload_ = std::make_unique<UploadBuffer<uint32_t>>(m_device_.Get(), 1, false);
	sortCounterUpload_ = std::make_unique<UploadBuffer<uint32_t>>(m_device_.Get(), 1, false);

	for (int i = 0; i < PARTICLE_COUNT; i++) {
		DeadListUpload_->CopyData(i, i);
	}
	deadCounterUpload_->CopyData(0, PARTICLE_COUNT);
	sortCounterUpload_->CopyData(0, 0);
}

void DX12App::InitUAVBuffers()
{
	UINT byteSize = PARTICLE_COUNT * sizeof(Particle);
	D3D12_RESOURCE_DESC uavDesc = {};
	uavDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.MipLevels = 1;
	uavDesc.Alignment = 0;
	uavDesc.DepthOrArraySize = 1;
	uavDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	uavDesc.Width = byteSize;
	uavDesc.Height = 1;
	uavDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	uavDesc.SampleDesc.Count = 1;
	uavDesc.SampleDesc.Quality = 0;

	CD3DX12_HEAP_PROPERTIES defaultHeapProp(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(m_device_->CreateCommittedResource(&defaultHeapProp, D3D12_HEAP_FLAG_NONE, &uavDesc, D3D12_RESOURCE_STATE_COMMON,
		nullptr, IID_PPV_ARGS(&RWParticleBuffer_)));
	ThrowIfFailed(m_device_->CreateCommittedResource(&defaultHeapProp, D3D12_HEAP_FLAG_NONE, &uavDesc, D3D12_RESOURCE_STATE_COMMON,
		nullptr, IID_PPV_ARGS(&ParticleDeadList_)));
	ThrowIfFailed(m_device_->CreateCommittedResource(&defaultHeapProp, D3D12_HEAP_FLAG_NONE, &uavDesc, D3D12_RESOURCE_STATE_COMMON,
		nullptr, IID_PPV_ARGS(&ParticleSortList_)));

	auto CounterDesc = CD3DX12_RESOURCE_DESC::Buffer(4, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ThrowIfFailed(m_device_->CreateCommittedResource(&defaultHeapProp, D3D12_HEAP_FLAG_NONE, &CounterDesc, D3D12_RESOURCE_STATE_COMMON,
		nullptr, IID_PPV_ARGS(&deadCounterBuffer_)));
	ThrowIfFailed(m_device_->CreateCommittedResource(&defaultHeapProp, D3D12_HEAP_FLAG_NONE, &CounterDesc, D3D12_RESOURCE_STATE_COMMON,
		nullptr, IID_PPV_ARGS(&sortCounterBuffer_)));

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavView = {};
	uavView.Buffer.NumElements = PARTICLE_COUNT;
	uavView.Buffer.FirstElement = 0;
	uavView.Buffer.StructureByteStride = sizeof(uint32_t);
	uavView.Buffer.CounterOffsetInBytes = 0;
	uavView.Format = DXGI_FORMAT_UNKNOWN;
	uavView.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavView.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	auto uavHandle = UAVHeap_->GetCPUDescriptorHandleForHeapStart();
	auto size = m_device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE Handle(uavHandle, 0, size);
	m_device_->CreateUnorderedAccessView(ParticleDeadList_.Get(), deadCounterBuffer_.Get(), &uavView, Handle);

	uavView.Buffer.StructureByteStride = sizeof(SortParticle);
	uavView.Buffer.CounterOffsetInBytes = 0;
	CD3DX12_CPU_DESCRIPTOR_HANDLE SortHandle(uavHandle, 1, size);
	m_device_->CreateUnorderedAccessView(ParticleSortList_.Get(), sortCounterBuffer_.Get(), &uavView, SortHandle);

	ThrowIfFailed(m_command_list_->Reset(m_direct_cmd_list_alloc_.Get(), nullptr));
	CD3DX12_RESOURCE_BARRIER deadListToCopy = CD3DX12_RESOURCE_BARRIER::Transition(ParticleDeadList_.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	CD3DX12_RESOURCE_BARRIER counterToCopy = CD3DX12_RESOURCE_BARRIER::Transition(deadCounterBuffer_.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	D3D12_RESOURCE_BARRIER resourceBarrier[] = {deadListToCopy, counterToCopy};
	m_command_list_->ResourceBarrier(_countof(resourceBarrier), resourceBarrier);
	m_command_list_->CopyResource(ParticleDeadList_.Get(), DeadListUpload_->Resource());
	m_command_list_->CopyResource(deadCounterBuffer_.Get(), deadCounterUpload_->Resource());
	CD3DX12_RESOURCE_BARRIER toSRV = CD3DX12_RESOURCE_BARRIER::Transition(RWParticleBuffer_.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	CD3DX12_RESOURCE_BARRIER deadListToUAV = CD3DX12_RESOURCE_BARRIER::Transition(ParticleDeadList_.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	CD3DX12_RESOURCE_BARRIER deadCounterToUAV = CD3DX12_RESOURCE_BARRIER::Transition(deadCounterBuffer_.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	CD3DX12_RESOURCE_BARRIER sortCounterToUAV = CD3DX12_RESOURCE_BARRIER::Transition(sortCounterBuffer_.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	D3D12_RESOURCE_BARRIER Barriers[] = {toSRV, deadListToUAV, deadCounterToUAV, sortCounterToUAV};
	m_command_list_->ResourceBarrier(_countof(Barriers), Barriers);
	m_command_list_->Close();
	ID3D12CommandList* cmdLists[] = {m_command_list_.Get()};
	m_command_queue_->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	FlushCommandQueue();
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

