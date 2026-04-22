#include "DX12App.h"

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