#include "DX12App.h"

void DX12App::CreateSOBuffers() {
	UINT maxTess = 3;
	UINT maxVerticesPerPatch = (maxTess * maxTess) * 3;
	UINT totalMaxVertices = 583680 * maxVerticesPerPatch;
	UINT64 soBufferSize = totalMaxVertices * sizeof(BakedVertex);

	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto SOBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(soBufferSize, D3D12_RESOURCE_FLAG_NONE);
	ThrowIfFailed(device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE,
		&SOBufferDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&streamOutputBuffer)));

	SOBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(8, D3D12_RESOURCE_FLAG_NONE);
	ThrowIfFailed(device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE,
		&SOBufferDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&filledSizeBuffer)));

	heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
	ThrowIfFailed(device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE,
		&SOBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readbackBuffer)));

	CD3DX12_RESOURCE_BARRIER soBarrier = CD3DX12_RESOURCE_BARRIER::Transition(streamOutputBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_STREAM_OUT);
	CD3DX12_RESOURCE_BARRIER fsBarrier = CD3DX12_RESOURCE_BARRIER::Transition(filledSizeBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_STREAM_OUT);
	D3D12_RESOURCE_BARRIER barriers[] = { soBarrier, fsBarrier };
	commandList->Reset(commandAllocator.Get(), nullptr);
	commandList->ResourceBarrier(2, barriers);
	commandList->Close();
	ID3D12CommandList* cmdsLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	FlushCommandQueue();

	streamOutputBufferView.BufferLocation = streamOutputBuffer->GetGPUVirtualAddress();
	streamOutputBufferView.SizeInBytes = soBufferSize;
	streamOutputBufferView.BufferFilledSizeLocation = filledSizeBuffer->GetGPUVirtualAddress();

}

void DX12App::DrawToStreamOutput(ComPtr<ID3D12GraphicsCommandList> m_command_list_)
{
	m_command_list_->SetPipelineState(renderSystem->streamOutputPSO_.Get());
	m_command_list_->SetGraphicsRootSignature(renderSystem->streamOutputRS_.Get());
	m_command_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
	ID3D12DescriptorHeap* descriptorHeaps[] = { cbvSrvHeap.Get(), samplerHeap.Get() };
	m_command_list_->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	m_command_list_->IASetVertexBuffers(0, 1, &vertexBuffers[0]);
	m_command_list_->IASetIndexBuffer(&indexBufferView);
	CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(
		cbvSrvHeap->GetGPUDescriptorHandleForHeapStart());

	m_command_list_->SetGraphicsRootDescriptorTable(0, cbvHandle);

	m_command_list_->SetGraphicsRootDescriptorTable(
		2,
		samplerHeap->GetGPUDescriptorHandleForHeapStart());

	m_command_list_->SetGraphicsRootConstantBufferView(6, hullBuffer->Resource()->GetGPUVirtualAddress());

	for (auto& sm : submeshes) {
		if (sm.name_.find("Sketchfab") != std::string::npos) {
			streamOutputMesh = sm;
			break;
		}
	}
	UINT matIndex = streamOutputMesh.materialIndex;
	UINT matSize = d3dUtil::CalcConstantBufferSize(sizeof(MaterialConstants));
	D3D12_GPU_VIRTUAL_ADDRESS matAddress = materialBuffer->Resource()->GetGPUVirtualAddress() + matIndex * matSize;
	m_command_list_->SetGraphicsRootConstantBufferView(3, matAddress);

	int texHeapIndex = materialData[matIndex].diffuseTextureIndex + 1;

	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(
		cbvSrvHeap->GetGPUDescriptorHandleForHeapStart(),
		texHeapIndex,
		cbvDescriptorSize);

	m_command_list_->SetGraphicsRootDescriptorTable(1, srvHandle);

	int normHeapIndex = materialData[matIndex].normalTextureIndex + 1;

	CD3DX12_GPU_DESCRIPTOR_HANDLE normHandle(
		cbvSrvHeap->GetGPUDescriptorHandleForHeapStart(),
		normHeapIndex,
		cbvDescriptorSize);

	int dispHeapIndex = materialData[matIndex].displacementTextureIndex + 1;

	CD3DX12_GPU_DESCRIPTOR_HANDLE dispHandle(
		cbvSrvHeap->GetGPUDescriptorHandleForHeapStart(),
		dispHeapIndex,
		cbvDescriptorSize);
	m_command_list_->SetGraphicsRootDescriptorTable(5, dispHandle);

	m_command_list_->SetGraphicsRootDescriptorTable(4, normHandle);

	for (int i = 0; i < streamOutputMesh.InstanceCount; i++) {
		instanceBuffer->CopyData(i, streamOutputMesh.instances[i]);
	}

	m_command_list_->SetGraphicsRootShaderResourceView(7, instanceBuffer->Resource()->GetGPUVirtualAddress());

	D3D12_STREAM_OUTPUT_BUFFER_VIEW soViews[] = { streamOutputBufferView };
	m_command_list_->SOSetTargets(0, 1, soViews);
	m_command_list_->DrawIndexedInstanced(streamOutputMesh.indexCount, 1, streamOutputMesh.startIndiceIndex, streamOutputMesh.startVerticeIndex, 0);
	m_command_list_->SOSetTargets(0, 1, nullptr);
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(filledSizeBuffer.Get(), D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_command_list_->ResourceBarrier(1, &barrier);
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		streamOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_STREAM_OUT,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	m_command_list_->ResourceBarrier(1, &barrier);
	m_command_list_->CopyResource(readbackBuffer.Get(), filledSizeBuffer.Get());
	m_command_list_->Close();
	ID3D12CommandList* cmdsLists[] = { m_command_list_.Get() };
	commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	FlushCommandQueue();

	UINT64* mappedData = nullptr;
	readbackBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));
	UINT64 filledBytes = *mappedData;
	readbackBuffer->Unmap(0, nullptr);

	streamOutputMesh.SOVertexCount = filledBytes / sizeof(BakedVertex);
}