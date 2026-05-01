#include "DX12App.h"
#include <numeric>

#define ALIGN_256(size) ((size + 255) & ~255)

void DX12App::InitRenderSystem() {
	renderSystem = new RenderingSystem(m_device_, m_client_width_, m_client_height_);

	CreateStructuredBuffersSRV();

	//Vector3 lightDir = renderSystem->sceneLights_[0].lightDirection;
	//Vector3 targetPos = camera.mCameraPos;
	////Vector3 lightPos = targetPos - lightDir * 500.0f;
	//Vector3 lightPos = Vector3(-50.0f, 5000.0f, 0.0f);
	//Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
	//Matrix lightView = Matrix::CreateLookAt(lightPos, targetPos, up);

	//float width = 15000.0f;
	//float height = 15000.0f;
	//float nearZ = 1.0f;
	//float farZ = 15000.0f;
	//ShadowConstants shadowData;
	//Matrix lightProj = Matrix::CreateOrthographic(width, height, nearZ, farZ);
	//shadowData.lightViewProj = lightView * lightProj;

	//Matrix T(
	//	0.5f, 0.0f, 0.0f, 0.0f,
	//	0.0f, -0.5f, 0.0f, 0.0f,
	//	0.0f, 0.0f, 1.0f, 0.0f,
	//	0.5f, 0.5f, 0.0f, 1.0f);


	//Matrix shadowTransform = lightView * lightProj * T;
	//shadowData.shadowTransform_ = shadowTransform;
	//shadowData.SMAP_SIZE = SMAP_SIZE;
	//ShadowCB->CopyData(0, shadowData);
}

void DX12App::DrawShadows(ComPtr<ID3D12GraphicsCommandList> m_command_list_) {
	m_command_list_->SetPipelineState(renderSystem->shadowPSO_.Get());
	m_command_list_->SetGraphicsRootSignature(renderSystem->shadowRS_.Get());
	m_command_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_command_list_->IASetVertexBuffers(0, 1, &VertexBuffers[0]);
	m_command_list_->IASetIndexBuffer(&ibv);

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(shadowMap_->Resource(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	m_command_list_->ResourceBarrier(1, &barrier);

	D3D12_VIEWPORT vp = shadowMap_->Viewport();
	m_command_list_->RSSetViewports(1, &vp);
	D3D12_RECT rect = shadowMap_->ScissorRect();
	m_command_list_->RSSetScissorRects(1, &rect);

	int numCascades = shadowMap_->GetNumCascades();
	UINT elementSize = ALIGN_256(sizeof(ShadowConstants));

	for (int i = 0; i < numCascades; ++i) {
		m_command_list_->ClearDepthStencilView(shadowMap_->Dsv(i), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsv = shadowMap_->Dsv(i);
		m_command_list_->OMSetRenderTargets(0, nullptr, false, &dsv);

		auto address = ShadowCB->Resource()->GetGPUVirtualAddress();
		address += i * elementSize;
		m_command_list_->SetGraphicsRootConstantBufferView(0, address);
		UINT currentInstanceOffset = 0;
		for (auto& sm : mSubmeshes) {
			for (int i = 0; i < sm.InstanceCount; i++) {
				InstanceBuffer->CopyData(currentInstanceOffset + i, sm.instances[i]);
			}
			m_command_list_->SetGraphicsRootShaderResourceView(1, InstanceBuffer->Resource()->GetGPUVirtualAddress());
			m_command_list_->DrawIndexedInstanced(
				sm.indexCount,
				sm.InstanceCount,
				sm.startIndiceIndex,
				sm.startVerticeIndex,
				currentInstanceOffset);
		}
	}
	CD3DX12_RESOURCE_BARRIER backBarrier = CD3DX12_RESOURCE_BARRIER::Transition(shadowMap_->Resource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	m_command_list_->ResourceBarrier(1, &backBarrier);
}

void DX12App::DrawToGBuffer(ComPtr<ID3D12GraphicsCommandList> m_command_list_) {
	visibleIndices.clear();

	if (camera.isFrustumCullingEnabled)
	{
		octree.GetVisibleObjects(camera.planes, mSubmeshes, visibleIndices);
	}
	else
	{
		visibleIndices.resize(mSubmeshes.size());
		std::iota(visibleIndices.begin(), visibleIndices.end(), 0);
	}

	m_command_list_->SetPipelineState(renderSystem->opaquePSO_.Get());

	m_command_list_->ClearDepthStencilView(GetDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

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

	m_command_list_->SetGraphicsRootConstantBufferView(6, HullCB->Resource()->GetGPUVirtualAddress());

	UINT currentInstanceOffset = 0;
	for (UINT idx : visibleIndices)
	{
		auto& sm = mSubmeshes[idx];
		for (int i = 0; i < sm.InstanceCount; i++) {
			InstanceBuffer->CopyData(currentInstanceOffset + i, sm.instances[i]);
		}
		m_command_list_->SetGraphicsRootShaderResourceView(7, InstanceBuffer->Resource()->GetGPUVirtualAddress());

		UINT matIndex = sm.materialIndex;
		UINT matSize = d3dUtil::CalcConstantBufferSize(sizeof(MaterialConstants));
		D3D12_GPU_VIRTUAL_ADDRESS matAddress = MaterialCB->Resource()->GetGPUVirtualAddress() + matIndex * matSize;
		m_command_list_->SetGraphicsRootConstantBufferView(3, matAddress);

		if (materialData[matIndex].isTree == 1) treeIsVisible = true;
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

		if (sm.name_.find("Sketchfab") != std::string::npos)
		{
			m_command_list_->SetPipelineState(renderSystem->bakedPSO_.Get());
			D3D12_VERTEX_BUFFER_VIEW bakedVbv;
			bakedVbv.BufferLocation = SOBuffer_->GetGPUVirtualAddress();
			bakedVbv.StrideInBytes = sizeof(BakedVertex);
			bakedVbv.SizeInBytes = SOMesh.SOVertexCount * sizeof(BakedVertex);

			m_command_list_->IASetVertexBuffers(0, 1, &bakedVbv);
			m_command_list_->DrawInstanced(
				SOMesh.SOVertexCount,
				sm.InstanceCount,
				0,
				currentInstanceOffset);
			m_command_list_->IASetVertexBuffers(0, 1, &VertexBuffers[0]);
		}
		else {
			m_command_list_->DrawIndexedInstanced(
				sm.indexCount,
				sm.InstanceCount,
				sm.startIndiceIndex,
				sm.startVerticeIndex,
				currentInstanceOffset);
		}

		currentInstanceOffset += sm.InstanceCount;

		if (sm.name_.find("Sketchfab") != std::string::npos)
		{
			m_command_list_->SetPipelineState(renderSystem->opaquePSO_.Get());
		}
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
	m_command_list_->SetGraphicsRootDescriptorTable(4, shadowMap_->Srv());
	m_command_list_->SetGraphicsRootConstantBufferView(5, ShadowCB->Resource()->GetGPUVirtualAddress());

	m_command_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_command_list_->DrawInstanced(3, 1, 0, 0);
	CD3DX12_RESOURCE_BARRIER backBarrier = CD3DX12_RESOURCE_BARRIER::Transition(shadowMap_->Resource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_GENERIC_READ);
	m_command_list_->ResourceBarrier(1, &backBarrier);
}

void DX12App::DrawNYBalls()
{
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
}



void DX12App::Draw()
{
	ThrowIfFailed(m_direct_cmd_list_alloc_->Reset());
	ThrowIfFailed(m_command_list_->Reset(m_direct_cmd_list_alloc_.Get(), renderSystem->opaquePSO_.Get()));
	DrawShadows(m_command_list_);
	m_command_list_->RSSetViewports(1, &vp_);
	m_command_list_->RSSetScissorRects(1, &m_scissor_rect_);

	renderSystem->g_buffer->TransitToOpaqueRenderingState(m_command_list_);

	m_command_list_->ClearRenderTargetView(renderSystem->g_buffer->DiffuseTex.rtvHandle, Color(0.0f, 0.0f, 0.0f, 1.0f), 0, nullptr);
	m_command_list_->ClearRenderTargetView(renderSystem->g_buffer->NormalTex.rtvHandle, Color(0.0f, 0.0f, 0.0f, 1.0f), 0, nullptr);
	m_command_list_->ClearDepthStencilView(renderSystem->g_buffer->DepthTex.dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	treeIsVisible = false;

	if (isFirstFrame) {
		DrawToStreamOutput(m_command_list_);
		ThrowIfFailed(m_command_list_->Reset(m_direct_cmd_list_alloc_.Get(), renderSystem->opaquePSO_.Get()));
		isFirstFrame = false;
	}
	DrawToGBuffer(m_command_list_);

	renderSystem->g_buffer->TransitToLightsRenderingState(m_command_list_);
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_command_list_->ResourceBarrier(1, &barrier);
	DrawLights(m_command_list_);

	if (treeIsVisible) DrawNYBalls();

	bool isEmitterInside = true;
	for (int i = 0; i < 6; ++i) {
		PlaneIntersectionType type = emitter.bounds.Intersects(camera.planes[i]);
		if (type == PlaneIntersectionType::BACK) {
			isEmitterInside = false;
			break;
		}
	}
	DrawWireframe(m_command_list_);
	EmitParticles();
	ComputeParticles();
	if (isEmitterInside) {
		DrawParticles(m_command_list_);
	}
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
