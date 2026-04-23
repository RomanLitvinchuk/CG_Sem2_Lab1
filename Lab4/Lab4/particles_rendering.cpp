#include "DX12App.h"

void DX12App::EmitParticles() {
	m_command_list_->SetPipelineState(renderSystem->particlesEmitPSO_.Get());
	m_command_list_->SetComputeRootSignature(renderSystem->particlesEmitRS_.Get());
	ID3D12DescriptorHeap* heaps[] = { UAVHeap_.Get() };
	m_command_list_->SetDescriptorHeaps(_countof(heaps), heaps);
	CD3DX12_RESOURCE_BARRIER toUAV = CD3DX12_RESOURCE_BARRIER::Transition(RWParticleBuffer_.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	D3D12_RESOURCE_BARRIER resourceBarrier = { toUAV };
	m_command_list_->ResourceBarrier(1, &resourceBarrier);
	m_command_list_->SetComputeRootUnorderedAccessView(0, RWParticleBuffer_->GetGPUVirtualAddress());
	CD3DX12_GPU_DESCRIPTOR_HANDLE uavHandle(
		UAVHeap_->GetGPUDescriptorHandleForHeapStart(),
		0,
		m_CbvSrvUav_descriptor_size_);
	m_command_list_->SetComputeRootDescriptorTable(1, uavHandle);
	m_command_list_->Dispatch(1, 1, 1);
	CD3DX12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(RWParticleBuffer_.Get());
	m_command_list_->ResourceBarrier(1, &uavBarrier);
}

void DX12App::ComputeParticles() {
	m_command_list_->SetPipelineState(renderSystem->particlesUpdatePSO_.Get());
	m_command_list_->SetComputeRootSignature(renderSystem->particlesUpdateRS_.Get());
	m_command_list_->SetComputeRootConstantBufferView(0, TimeBuffer->Resource()->GetGPUVirtualAddress());
	m_command_list_->SetComputeRootUnorderedAccessView(1, RWParticleBuffer_->GetGPUVirtualAddress());
	CD3DX12_GPU_DESCRIPTOR_HANDLE uavHandle(
		UAVHeap_->GetGPUDescriptorHandleForHeapStart(),
		0,
		m_CbvSrvUav_descriptor_size_);
	m_command_list_->SetComputeRootDescriptorTable(2, uavHandle);

	m_command_list_->Dispatch(ceil(PARTICLE_COUNT / 256.0f), 1, 1);
	CD3DX12_RESOURCE_BARRIER toCopy = CD3DX12_RESOURCE_BARRIER::Transition(RWParticleBuffer_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	D3D12_RESOURCE_BARRIER resourceBarrier = { toCopy };
	m_command_list_->ResourceBarrier(1, &resourceBarrier);
}

void DX12App::DrawParticles(ComPtr<ID3D12GraphicsCommandList> m_command_list) {
	m_command_list_->SetPipelineState(renderSystem->particlePSO_.Get());
	m_command_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

	D3D12_CPU_DESCRIPTOR_HANDLE bb = GetBackBuffer();
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = renderSystem->g_buffer->DepthTex.dsvHandle;
	m_command_list_->OMSetRenderTargets(1, &bb, true, &dsv);

	m_command_list_->SetGraphicsRootSignature(renderSystem->particleRS_.Get());
	
	m_command_list_->SetGraphicsRootConstantBufferView(0, MatricesBuffer->Resource()->GetGPUVirtualAddress());
	m_command_list_->SetGraphicsRootShaderResourceView(1, RWParticleBuffer_->GetGPUVirtualAddress());

	m_command_list_->IASetVertexBuffers(0, 0, nullptr);
	m_command_list_->IASetIndexBuffer(nullptr);

	m_command_list_->DrawInstanced(1, PARTICLE_COUNT, 0, 0);
}