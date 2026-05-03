#ifndef SHADOW_MAP_
#define SHADOW_MAP_
#include <d3d12.h>
#include <wrl.h>
#include <d3dx12.h>
#include <DirectXMath.h>
#include <SimpleMath.h>

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace SimpleMath;

class ShadowMap
{
public:
    ShadowMap(ID3D12Device* device,
        UINT width, UINT height);
    ShadowMap(const ShadowMap& rhs) = delete;
    ShadowMap& operator=(const ShadowMap& rhs) = delete;

    UINT Width()const;
    UINT Height()const;
    ID3D12Resource* Resource();
    ID3D12Resource* ColorResource();
    CD3DX12_GPU_DESCRIPTOR_HANDLE Srv()const;
    CD3DX12_CPU_DESCRIPTOR_HANDLE Dsv(int index)const;

    CD3DX12_GPU_DESCRIPTOR_HANDLE ColorSrv() const;
    CD3DX12_CPU_DESCRIPTOR_HANDLE Rtv(int index) const;

    D3D12_VIEWPORT Viewport()const;
    D3D12_RECT ScissorRect()const;

    void BuildDescriptors(
        CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
        CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
        CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv,
        CD3DX12_CPU_DESCRIPTOR_HANDLE hColorCPUSrv,
        CD3DX12_GPU_DESCRIPTOR_HANDLE hColorGPUSrv,
        CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv);

    void OnResize(UINT newWidth, UINT newHeight);
    int GetNumCascades() const { return NUM_CASCADES; }
private:
    void BuildDescriptors();
    void BuildResource();

private:
    int NUM_CASCADES = 3;
    ID3D12Device* md3dDevice = nullptr;
    D3D12_VIEWPORT mViewport;
    D3D12_RECT mScissorRect;
    UINT mWidth = 0;
    UINT mHeight = 0;
    DXGI_FORMAT mFormat = DXGI_FORMAT_R24G8_TYPELESS;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuSrv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuDsv[3];
    ComPtr<ID3D12Resource> mShadowMap = nullptr;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mhColorCpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mhColorGpuSrv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuRtv[3];
    ComPtr<ID3D12Resource> mColorShadowMap = nullptr;
};

struct ShadowConstants {
    Matrix lightViewProj;
    Matrix shadowTransform_;
    Vector4 cascadeDistances;
};

struct CascadeData {
    Matrix viewProjMats[3];
    Matrix shadowTransform[3];
    float distances[3];
};

#endif //SHADOW_MAP
