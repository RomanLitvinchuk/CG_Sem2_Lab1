#include "DX12App.h"

void DX12App::DrawParticles(ComPtr<ID3D12GraphicsCommandList> m_command_list) {
	m_command_list_->SetPipelineState(renderSystem->particlePSO_.Get());
	m_command_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

	D3D12_CPU_DESCRIPTOR_HANDLE bb = GetBackBuffer();
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = renderSystem->g_buffer->DepthTex.dsvHandle;
	m_command_list_->OMSetRenderTargets(1, &bb, true, &dsv);

	m_command_list_->SetGraphicsRootSignature(renderSystem->particleRS_.Get());
	
	m_command_list_->SetGraphicsRootConstantBufferView(0, MatricesBuffer->Resource()->GetGPUVirtualAddress());
	m_command_list_->SetGraphicsRootShaderResourceView(1, ParticleBuffer->Resource()->GetGPUVirtualAddress());

	m_command_list_->IASetVertexBuffers(0, 0, nullptr);
	m_command_list_->IASetIndexBuffer(nullptr);

	m_command_list_->DrawInstanced(1, PARTICLE_COUNT, 0, 0);
}