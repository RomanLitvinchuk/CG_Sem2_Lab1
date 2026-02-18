#include "DX12App.h"
#include "d3dx12.h"
#include <iostream>
#include <string>
#include <DirectXColors.h>
#include <SimpleMath.h>
#include "d3dUtil.h"
#include "vertex.h"
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
	D3D12_DESCRIPTOR_HEAP_DESC CBVHeapDesc;
	CBVHeapDesc.NumDescriptors = 1;
	CBVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	CBVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	CBVHeapDesc.NodeMask = 0;
	ThrowIfFailed(m_device_->CreateDescriptorHeap(&CBVHeapDesc, IID_PPV_ARGS(&m_CBV_heap_)));
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

void DX12App::CreateSRV() {
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 3;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(m_device_->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_SRV_heap_)));
	std::cout << "SRV heap is created" << std::endl;

	auto cargoTex = std::make_unique<Texture>();
	cargoTex->name_ = "cargoTex";
	cargoTex->filepath = L"textures/texture.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(m_device_.Get(), m_command_list_.Get(),
		cargoTex->filepath.c_str(), cargoTex->Resource, cargoTex->UploadHeap));
	std::cout << "Texture is loaded" << std::endl;


	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(m_SRV_heap_->GetCPUDescriptorHandleForHeapStart());
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = cargoTex->Resource->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = cargoTex->Resource->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	m_device_->CreateShaderResourceView(cargoTex->Resource.Get(), &srvDesc, hDescriptor);
	std::cout << "SRV is created" << std::endl;
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


void DX12App::Draw(const GameTimer& gt)
{
	ThrowIfFailed(m_direct_cmd_list_alloc_->Reset());

	ThrowIfFailed(m_command_list_->Reset(m_direct_cmd_list_alloc_.Get(), PSO_.Get()));

	m_command_list_->RSSetViewports(1, &vp_);
	m_command_list_->RSSetScissorRects(1, &m_scissor_rect_);

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), 
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_command_list_->ResourceBarrier(1, &barrier);

	m_command_list_->ClearRenderTargetView(GetBackBuffer(), Colors::LightSteelBlue, 0, nullptr);
	m_command_list_->ClearDepthStencilView(GetDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	D3D12_CPU_DESCRIPTOR_HANDLE bb = GetBackBuffer();
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = GetDSV();
	m_command_list_->OMSetRenderTargets(1, &bb, true, &dsv);

	ID3D12DescriptorHeap* descriptorHeaps[] = { m_CBV_heap_.Get(), m_SRV_heap_.Get()};
	m_command_list_->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	m_command_list_->SetGraphicsRootSignature(m_root_signature_.Get());

	m_command_list_->IASetVertexBuffers(0, 1, &VertexBuffers[0]);
	m_command_list_->IASetIndexBuffer(&ibv);
	m_command_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_command_list_->SetGraphicsRootDescriptorTable(0, m_CBV_heap_->GetGPUDescriptorHandleForHeapStart());
	m_command_list_->SetGraphicsRootDescriptorTable(1, m_SRV_heap_->GetGPUDescriptorHandleForHeapStart());

	m_command_list_->DrawIndexedInstanced(
		indices.size(),
		1, 0, 0, 0);

	CD3DX12_RESOURCE_BARRIER barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	m_command_list_->ResourceBarrier(1, &barrier2);

	ThrowIfFailed(m_command_list_->Close());

	ID3D12CommandList* cmdsLists[] = { m_command_list_.Get() };
	m_command_queue_->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	ThrowIfFailed(m_swap_chain_->Present(0, 0));
	m_current_back_buffer_ = (m_current_back_buffer_ + 1) % 2;

	FlushCommandQueue();
}

void DX12App::SetTopology() {
	m_command_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void DX12App::InitProjectionMatrix() {
	float aspectRatio = static_cast<float>(m_client_width_) / m_client_height_;

	mProj_ = Matrix::CreatePerspectiveFieldOfView(
		XMConvertToRadians(60.0f),  
		aspectRatio,                
		0.01f,                       
		100000.0f                    
	);

	std::cout << "Projection matrix initialized. Aspect ratio: "
		<< aspectRatio << std::endl;
	
}


void DX12App::CreateVertexBuffer() {
	//Vertex vertices[] =
	//{
	//	{ Vector3(-1.0f, -1.0f, -1.0f), Vector4(Colors::Green) },
	//	//{ Vector3(-1.0f, +1.0f, -1.0f), Vector4(Colors::Black) },
	//	//{ Vector3(+1.0f, +1.0f, -1.0f), Vector4(Colors::Red) },
	//	{ Vector3(+1.0f, -1.0f, -1.0f), Vector4(Colors::Green) },
	//	{ Vector3(-1.0f, -1.0f, +1.0f), Vector4(Colors::Green) },
	//	//{ Vector3(-1.0f, +1.0f, +1.0f), Vector4(Colors::Yellow) },
	//	//{ Vector3(+1.0f, +1.0f, +1.0f), Vector4(Colors::Cyan) },
	//	{ Vector3(+1.0f, -1.0f, +1.0f), Vector4(Colors::Green) },
	//	{ Vector3(0.0f, +1.0f, 0.0f), Vector4(Colors::Red)}
	//};
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
	//std::uint16_t indices[] = {

	//	1, 2, 0,
	//	1, 3, 2,
	//	4, 1, 0,
	//	0, 2, 4,

	//	2, 3, 4,
	//	3, 1, 4
	//};
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
		//std::cout << "dx=" << std::to_string(dx) << " dy=" << std::to_string(dy) << std::endl;
		mTheta_ += XMConvertToRadians(0.25f * static_cast<float>(dx));
		mPhi_ += XMConvertToRadians(0.25f * static_cast<float>(dy));

		mPhi_ = mPhi_ < 0.1f ? 0.1f : (mPhi_ > XM_PI ? XM_PI : mPhi_);
		std::cout << "PHI:" << std::to_string(mPhi_) << " THETA:" << std::to_string(mTheta_);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		mRadius_ += 0.005f * static_cast<float>(dx - dy);

		mRadius_ = mRadius_ < 3.0f ? 3.0f : (mRadius_ > 15.0f ? 15.0f : mRadius_);
	}
}

void DX12App::Update(const GameTimer& gt) {
	std::cout << "Phi_" << std::to_string(mPhi_) << "Theta" << std::to_string(mTheta_) << std::endl;
	float x = mRadius_ * sinf(mPhi_) * cosf(mTheta_);
	float z = mRadius_ * sinf(mPhi_) * sinf(mTheta_);
	float y = mRadius_ * cosf(mPhi_);


	Vector3 pos(x, y, z);
	Vector3 target(0.0f, 0.0f, 0.0f);
	Vector3 up(0.0f, 1.0f, 0.0f);

	mView_ = Matrix::CreateLookAt(pos, target, up);


	Matrix WorldViewProj = mWorld_ * mView_ * mProj_;
	WorldViewProj = WorldViewProj.Transpose();


	ObjectConstants obj;
	obj.mWorldViewProj = WorldViewProj;

	CBUploadBuffer->CopyData(0, obj);
}

void DX12App::InitUploadBuffer() {
	CBUploadBuffer = std::make_unique<UploadBuffer<ObjectConstants>>(
		m_device_.Get(),
		1,
		true
	);
}

void DX12App::CreateConstantBufferView() {
	UINT cbByteSize = d3dUtil::CalcConstantBufferSize(sizeof(ObjectConstants));
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = CBUploadBuffer->Resource()->GetGPUVirtualAddress();
	int BoxCBIndex = 0;
	cbAddress += BoxCBIndex * cbByteSize;
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbDesc;
	cbDesc.BufferLocation = cbAddress;
	cbDesc.SizeInBytes = cbByteSize;
	m_device_->CreateConstantBufferView(&cbDesc, m_CBV_heap_->GetCPUDescriptorHandleForHeapStart());
	std::cout << "Constant buffer view is created" << std::endl;
}

void DX12App::CreateRootSignature() {
	CD3DX12_ROOT_PARAMETER slotRootParameter[2];
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	CD3DX12_DESCRIPTOR_RANGE srvTable;
	srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);
	slotRootParameter[1].InitAsDescriptorTable(1, &srvTable);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		2, slotRootParameter,
		0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(
		&rootSigDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(),
		errorBlob.GetAddressOf()
	);

	if (errorBlob != nullptr) {
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}

	ThrowIfFailed(hr);
	ThrowIfFailed(m_device_->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&m_root_signature_)
	));

	std::cout << "Root Signature is created" << std::endl;
}

void DX12App::CompileShaders() {
	DWORD fileAttr = GetFileAttributes(L"shaders.hlsl");
	if (fileAttr == INVALID_FILE_ATTRIBUTES) {
		std::wcout << L"ERROR: Shader file not found!" << std::endl;
		MessageBox(NULL, L"Shader file not found!", L"Error", MB_OK);
		return;
	}
	mvsByteCode_ = d3dUtil::CompileShader(L"shaders.hlsl", nullptr, "VS", "vs_5_0");
	std::cout << "Vertex shader are compiled" << std::endl;
	mpsByteCode_ = d3dUtil::CompileShader(L"shaders.hlsl", nullptr, "PS", "ps_5_0");
	std::cout << "Pixel shader are compiled" << std::endl;
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

void DX12App::CreatePSO() {
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { m_input_layout_.data(), (UINT)m_input_layout_.size() };
	psoDesc.pRootSignature = m_root_signature_.Get();
	psoDesc.VS = { reinterpret_cast<BYTE*>(mvsByteCode_->GetBufferPointer()), mvsByteCode_->GetBufferSize() };
	psoDesc.PS = { reinterpret_cast<BYTE*>(mpsByteCode_->GetBufferPointer()), mpsByteCode_->GetBufferSize() };
	CD3DX12_RASTERIZER_DESC rastDesc(D3D12_DEFAULT);
	//rastDesc.CullMode = D3D12_CULL_MODE_NONE;
	rastDesc.FrontCounterClockwise = true; 
	psoDesc.RasterizerState = rastDesc;
	//psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = m_back_buffer_format_;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = m_depth_stencil_format_;
	ThrowIfFailed(m_device_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&PSO_)));
	std::cout << "PSO is created" << std::endl;
}

void DX12App::ParseFile() {
	sponza = aiImportFile("cargo.obj", aiProcessPreset_TargetRealtime_MaxQuality);
	std::cout << "Num Meshes : " << std::to_string(sponza->mNumMeshes) << std::endl;
	ParseNode(sponza->mRootNode, sponza, vertices, indices);
	std::cout << "sponza.obj is parsed" << std::endl;
	std::cout << "Num Vertices is:" << std::to_string(vertices.size()) << std::endl;
	std::cout << "Num Faces is: " << std::to_string(sponza->mMeshes[0]->mNumFaces) << std::endl;
	std::cout << "Num indices is:" << std::to_string(indices.size()) << std::endl;
}

void DX12App::ParseNode(aiNode* node, const aiScene* scene, std::vector<Vertex>& vertices, std::vector<std::uint32_t>& indices) {
	for (int i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		ParseMesh(mesh, vertices, indices);
	}

	for (int i = 0; i < node->mNumChildren; i++) {
		ParseNode(node->mChildren[i], scene, vertices, indices);
	}
}

void DX12App::ParseMesh(aiMesh* mesh, std::vector<Vertex>& vertices, std::vector<std::uint32_t>& indices) {
	size_t vertexOffset = vertices.size();

	for (int i = 0; i < mesh->mNumVertices; ++i) {
		Vertex vertex;

		vertex.pos.x = mesh->mVertices[i].x;
		vertex.pos.y = mesh->mVertices[i].y;
		vertex.pos.z = mesh->mVertices[i].z;

		if (mesh->HasNormals()) {
			vertex.normal.x = mesh->mNormals[i].x;
			vertex.normal.y = mesh->mNormals[i].y;
			vertex.normal.z = mesh->mNormals[i].z;
		}
		else {
			vertex.normal = Vector3(0.0f, 1.0f, 0.0f);
		}
		if (mesh->HasTextureCoords(0)) {
			vertex.uv.x = mesh->mTextureCoords[0][i].x;
			vertex.uv.y = 1.0f - mesh->mTextureCoords[0][i].y;
		}
		else {
			vertex.uv = Vector2(0.0f, 0.0f);
		}

		vertices.push_back(vertex);

	}
	for (int i = 0; i < mesh->mNumFaces; ++i) {
		aiFace face = mesh->mFaces[i];
		for (int j = 0; j < face.mNumIndices; ++j) {
			indices.push_back(face.mIndices[j] + vertexOffset);
		}
	}
}