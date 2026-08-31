#include "DX12App.h"

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

void DX12App::CreateSRV() {

	auto handle = m_CBV_SRV_heap_->GetCPUDescriptorHandleForHeapStart();

	for (auto& [name, tex] : mTextures)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE h(handle,
			tex->srvHeapIndex + 1,
			m_CbvSrvUav_descriptor_size_);

		D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		DXGI_FORMAT format = tex->Resource->GetDesc().Format;

		if (tex->isSRGB) {
			format = d3dUtil::MakeSRGB(format);
		}

		desc.Format = format;
		desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipLevels = tex->Resource->GetDesc().MipLevels;

		m_device_->CreateShaderResourceView(tex->Resource.Get(), &desc, h);
	}
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
	D3D12_RESOURCE_BARRIER resourceBarrier[] = { deadListToCopy, counterToCopy };
	m_command_list_->ResourceBarrier(_countof(resourceBarrier), resourceBarrier);
	m_command_list_->CopyResource(ParticleDeadList_.Get(), DeadListUpload_->Resource());
	m_command_list_->CopyResource(deadCounterBuffer_.Get(), deadCounterUpload_->Resource());
	CD3DX12_RESOURCE_BARRIER toSRV = CD3DX12_RESOURCE_BARRIER::Transition(RWParticleBuffer_.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	CD3DX12_RESOURCE_BARRIER deadListToUAV = CD3DX12_RESOURCE_BARRIER::Transition(ParticleDeadList_.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	CD3DX12_RESOURCE_BARRIER deadCounterToUAV = CD3DX12_RESOURCE_BARRIER::Transition(deadCounterBuffer_.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	CD3DX12_RESOURCE_BARRIER sortCounterToUAV = CD3DX12_RESOURCE_BARRIER::Transition(sortCounterBuffer_.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	D3D12_RESOURCE_BARRIER Barriers[] = { toSRV, deadListToUAV, deadCounterToUAV, sortCounterToUAV };
	m_command_list_->ResourceBarrier(_countof(Barriers), Barriers);
	m_command_list_->Close();
	ID3D12CommandList* cmdLists[] = { m_command_list_.Get() };
	m_command_queue_->ExecuteCommandLists(_countof(cmdLists), cmdLists);

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
