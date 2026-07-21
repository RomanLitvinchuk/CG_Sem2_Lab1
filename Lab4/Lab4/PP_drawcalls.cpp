#include "DX12App.h"

void DX12App::DrawPPTonemap(ComPtr<ID3D12GraphicsCommandList> m_command_list_) {
	m_command_list_->SetPipelineState(renderSystem->pp_tonemappingPSO_.Get());
	m_command_list_->SetGraphicsRootSignature(renderSystem->pp_tonemappingRS_.Get());

	m_command_list_->ClearRenderTargetView(GetBackBuffer(), Colors::Black, 0, nullptr);
	D3D12_CPU_DESCRIPTOR_HANDLE bb = GetBackBuffer();
	m_command_list_->OMSetRenderTargets(1, &bb, true, nullptr);

	ID3D12DescriptorHeap* descriptorHeaps[] = { renderSystem->post_process->srvHeap_.Get(), renderSystem->samplerHeap.Get() };
	m_command_list_->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	m_command_list_->SetGraphicsRootDescriptorTable(0, renderSystem->post_process->srvHeap_->GetGPUDescriptorHandleForHeapStart());
	m_command_list_->SetGraphicsRootDescriptorTable(1, renderSystem->samplerHeap->GetGPUDescriptorHandleForHeapStart());

	m_command_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_command_list_->DrawInstanced(3, 1, 0, 0);
}