#include "DX12App.h"

void DX12App::DrawWireframe(ComPtr<ID3D12GraphicsCommandList> m_command_list_)
{
	m_command_list_->SetPipelineState(renderSystem->wireframePSO_.Get());
	m_command_list_->SetGraphicsRootSignature(renderSystem->wireframeRS_.Get());
	m_command_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	
	m_command_list_->IASetVertexBuffers(0, 1, &wireframeVBV);
	m_command_list_->IASetIndexBuffer(&wireframeIBV);

	m_command_list_->SetGraphicsRootConstantBufferView(0, CBUploadBuffer->Resource()->GetGPUVirtualAddress());

	std::vector<BVHNode*> allNodes;
	octree.GetAllNodes(allNodes);

	if (allNodes.empty()) return;

	UINT numInstances = (std::min)((UINT)allNodes.size(), 1000U);

	for (UINT i = 0; i < numInstances; i++) {
		WireframeInstanceData data;

		data.center = allNodes[i]->bounds.Center;
		data.extents = allNodes[i]->bounds.Extents;
		data.color = Vector4(0.0f, 1.0f, 0.0f, 1.0f);
		WireframeInstanceBuffer->CopyData(i, data);
	}

	m_command_list_->SetGraphicsRootShaderResourceView(1, WireframeInstanceBuffer->Resource()->GetGPUVirtualAddress());

	m_command_list_->DrawIndexedInstanced(24, numInstances, 0, 0, 0);
}