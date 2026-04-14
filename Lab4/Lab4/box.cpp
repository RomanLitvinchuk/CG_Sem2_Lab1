#include "DX12App.h"

struct BoxVertex {
	Vector3 pos;
};

void DX12App::BuildBoxGeometry() {
    BoxVertex vertices[] = {
    { XMFLOAT3(-0.5f, -0.5f, -0.5f) }, 
    { XMFLOAT3(-0.5f,  0.5f, -0.5f) }, 
    { XMFLOAT3(0.5f,  0.5f, -0.5f) },
    { XMFLOAT3(0.5f, -0.5f, -0.5f) }, 
    { XMFLOAT3(-0.5f, -0.5f,  0.5f) }, 
    { XMFLOAT3(-0.5f,  0.5f,  0.5f) }, 
    { XMFLOAT3(0.5f,  0.5f,  0.5f) }, 
    { XMFLOAT3(0.5f, -0.5f,  0.5f) }  
    };

    uint16_t indices[] = {
        0, 1, 1, 2, 2, 3, 3, 0,
        4, 5, 5, 6, 6, 7, 7, 4,
        0, 4, 1, 5, 2, 6, 3, 7
    };

    ThrowIfFailed(m_direct_cmd_list_alloc_->Reset());
    ThrowIfFailed(m_command_list_->Reset(m_direct_cmd_list_alloc_.Get(), nullptr));

    const UINT vbByteSize = sizeof(vertices);
    const UINT ibByteSize = sizeof(indices);

    wireframeVB = d3dUtil::CreateDefaultBuffer(m_device_.Get(), m_command_list_.Get(), vertices, vbByteSize, VertexBufferUploader_);
    wireframeIB = d3dUtil::CreateDefaultBuffer(m_device_.Get(), m_command_list_.Get(), indices, ibByteSize, VertexBufferUploader_);

    ThrowIfFailed(m_command_list_->Close());
    ID3D12CommandList* cmdLists[] = { m_command_list_.Get() };
    m_command_queue_->ExecuteCommandLists(_countof(cmdLists), cmdLists);
    FlushCommandQueue();

    wireframeVBV.BufferLocation = wireframeVB->GetGPUVirtualAddress();
    wireframeVBV.StrideInBytes = sizeof(BoxVertex);
    wireframeVBV.SizeInBytes = vbByteSize;

    wireframeIBV.BufferLocation = wireframeIB->GetGPUVirtualAddress();
    wireframeIBV.SizeInBytes = ibByteSize;
    wireframeIBV.Format = DXGI_FORMAT_R16_UINT;
}