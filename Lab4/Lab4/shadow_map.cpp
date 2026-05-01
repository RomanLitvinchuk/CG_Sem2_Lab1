#include "shadow_map.h"
#include "throw_if_failed.h"
#include "DX12App.h"

ShadowMap::ShadowMap(ID3D12Device* device, UINT width, UINT height)
{
    md3dDevice = device;
    mWidth = width;
    mHeight = height;
    mViewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
    mScissorRect = { 0, 0, (int)width, (int)height };
    BuildResource();
}

void ShadowMap::BuildResource()
{
    D3D12_RESOURCE_DESC texDesc;
    ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Alignment = 0;
    texDesc.Width = mWidth;
    texDesc.Height = mHeight;
    texDesc.DepthOrArraySize = NUM_CASCADES;
    texDesc.MipLevels = 1;
    texDesc.Format = mFormat;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear;
    optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;
    
    auto heapDefault = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &heapDefault,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        &optClear,
        IID_PPV_ARGS(&mShadowMap)));
    mShadowMap.Get()->SetName(L"Shadow Map Texture");

}

ID3D12Resource* ShadowMap::Resource()
{
    return mShadowMap.Get();
}

CD3DX12_GPU_DESCRIPTOR_HANDLE ShadowMap::Srv()const
{
    return mhGpuSrv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE ShadowMap::Dsv(int index)const
{
    return mhCpuDsv[index];
}

D3D12_VIEWPORT ShadowMap::Viewport() const
{
    return mViewport;
}

D3D12_RECT ShadowMap::ScissorRect() const
{
    return mScissorRect;
}

void ShadowMap::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv)
{
    mhCpuSrv = hCpuSrv;
    mhGpuSrv = hGpuSrv;
    UINT dsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    for (int i = 0; i < NUM_CASCADES; ++i) {
        mhCpuDsv[i] = hCpuDsv;
        hCpuDsv.Offset(1, dsvDescriptorSize);
    }

    BuildDescriptors();
}

void ShadowMap::BuildDescriptors() {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Texture2DArray.MostDetailedMip = 0;
    srvDesc.Texture2DArray.MipLevels = 1;
    srvDesc.Texture2DArray.ArraySize = NUM_CASCADES;
    srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
    srvDesc.Texture2DArray.PlaneSlice = 0;

    md3dDevice->CreateShaderResourceView(mShadowMap.Get(), &srvDesc, mhCpuSrv);


    for (int i = 0; i < NUM_CASCADES; i++) {
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        dsvDesc.Texture2DArray.FirstArraySlice = i;
        dsvDesc.Texture2DArray.ArraySize = 1;
        dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dsvDesc.Texture2DArray.MipSlice = 0;

        md3dDevice->CreateDepthStencilView(mShadowMap.Get(), &dsvDesc, mhCpuDsv[i]);
    }
}

void ShadowMap::OnResize(UINT newWidth, UINT newHeight)
{
    if ((mWidth != newWidth) || (mHeight != newHeight))
    {
        mWidth = newWidth;
        mHeight = newHeight;

        mViewport = { 0.0f, 0.0f, (float)mWidth, (float)mHeight, 0.0f, 1.0f };
        mScissorRect = { 0, 0, (int)mWidth, (int)mHeight };

        BuildResource();

        BuildDescriptors();
    }
}

UINT ShadowMap::Width() const {
    return mWidth;
}

UINT ShadowMap::Height() const {
    return mHeight;
}

void DX12App::InitShadowMap() {
    shadowMap_ = std::make_unique<ShadowMap>(m_device_.Get(), SMAP_SIZE, SMAP_SIZE);

    auto handle = renderSystem->g_buffer->SRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    auto size = m_device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    CD3DX12_CPU_DESCRIPTOR_HANDLE smHandle(handle, 4, size);

    auto gpuHandle = renderSystem->g_buffer->SRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
    CD3DX12_GPU_DESCRIPTOR_HANDLE smGpuHandle(gpuHandle, 4, size);

    auto dsvHandle = m_DSV_heap_->GetCPUDescriptorHandleForHeapStart();
    size = m_device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    CD3DX12_CPU_DESCRIPTOR_HANDLE smDsvHandle(dsvHandle, 1, size);

    shadowMap_->BuildDescriptors(smHandle, smGpuHandle, smDsvHandle);
}