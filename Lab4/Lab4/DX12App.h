#ifndef DX12APP_
#define DX12APP_

#include <Windows.h>
#include <d3d12.h>
#include <DirectXHelpers.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <comdef.h>
#include <DescriptorHeap.h>
#include <d3dx12.h>
#include "throw_if_failed.h"
#include "game_timer.h"
#include "upload_buffer.h"
#include "object_constants.h"
#include "vertex.h"
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace Microsoft::WRL;
using namespace DirectX;

class DX12App
{
public:
	void InitializeDevice();
	void InitializeCommandObjects();
	void CreateSwapChain(HWND hWnd);
	void CreateRTVAndDSVDescriptorHeaps();
	void CreateCBVDescriptorHeap();
	D3D12_CPU_DESCRIPTOR_HANDLE GetBackBuffer() const;
	ID3D12Resource* CurrentBackBuffer() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const;
	void CreateRTV();
	void CreateDSV();
	void SetViewport();
	void SetScissor();

	void CalculateGameStats(GameTimer& gt, HWND hWnd);
	void Draw(const GameTimer& gt);

	void FlushCommandQueue();
	void SetTopology();

	void InitProjectionMatrix();
	void CreateVertexBuffer();
	void CreateIndexBuffer();

	void OnMouseDown(HWND hWnd);
	void OnMouseUp();
	void OnMouseMove(WPARAM btnState, int dx, int dy);

	void Update(const GameTimer& gt);

	void InitUploadBuffer();
	void CreateConstantBufferView();

	void CreateRootSignature();
	void CompileShaders();
	void BuildLayout();

	void CreatePSO();

	void ParseFile();
	void ParseNode(aiNode* node, const aiScene* scene, std::vector<Vertex>& vertices, std::vector<std::uint32_t>& indices);
	void ParseMesh(aiMesh* mesh, std::vector<Vertex>& vertices, std::vector<std::uint32_t>& indices);

	ComPtr<ID3D12Device> GetDevice() const { return m_device_; }
	ComPtr<ID3D12GraphicsCommandList> GetCommandList() const { return m_command_list_; }
private:
	void EnableDebug();

	DXGI_FORMAT m_back_buffer_format_ = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT m_depth_stencil_format_ = DXGI_FORMAT_D24_UNORM_S8_UINT;
	int m_client_width_ = 800;
	int m_client_height_ = 600;

	ComPtr<IDXGIFactory4> m_dxgi_factory_;
	ComPtr<ID3D12Device> m_device_;
	ComPtr<ID3D12Fence> m_fence_;
	UINT64 m_current_fence_ = 0;
	UINT m_RTV_descriptor_size_ = 0;
	UINT m_DSV_descriptor_size_ = 0;
	UINT m_CbvSrvUav_descriptor_size_ = 0;
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels_;
	

	ComPtr<ID3D12CommandQueue> m_command_queue_;
	ComPtr<ID3D12CommandAllocator> m_direct_cmd_list_alloc_;
	ComPtr<ID3D12GraphicsCommandList> m_command_list_;
	
	ComPtr<IDXGISwapChain> m_swap_chain_;

	ComPtr<ID3D12DescriptorHeap> m_RTV_heap_;
	ComPtr<ID3D12DescriptorHeap> m_DSV_heap_;
	ComPtr<ID3D12DescriptorHeap> m_CBV_heap_;
	int m_current_back_buffer_ = 0;

	ComPtr<ID3D12Resource> m_swap_chain_buffer_[2];
	ComPtr<ID3D12Resource> m_DSV_buffer;

	D3D12_VIEWPORT vp_;
	D3D12_RECT m_scissor_rect_;

	ComPtr<ID3D12Resource> VertexBufferGPU_ = nullptr;
	ComPtr<ID3D12Resource> VertexBufferUploader_ = nullptr;
	D3D12_VERTEX_BUFFER_VIEW VertexBuffers[1];

	ComPtr<ID3D12Resource> IndexBufferGPU_ = nullptr;
	ComPtr<ID3D12Resource> IndexBufferUploader_ = nullptr;
	D3D12_INDEX_BUFFER_VIEW ibv;


	std::unique_ptr<UploadBuffer<ObjectConstants>> CBUploadBuffer;

	ComPtr<ID3DBlob> serializedRootSig_ = nullptr;
	ComPtr<ID3DBlob> errorBlob_ = nullptr;
	ComPtr<ID3D12RootSignature> m_root_signature_;

	ComPtr<ID3DBlob> mvsByteCode_ = nullptr;
	ComPtr<ID3DBlob> mpsByteCode_ = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> m_input_layout_;

	ComPtr<ID3D12PipelineState> PSO_;

	Matrix mWorld_ = Matrix::Identity;
	Matrix mView_ = Matrix::Identity;
	Matrix mProj_ = Matrix::Identity;

	POINT m_mouse_last_pos_;
	float mTheta_ = XM_PIDIV4;   // 45°
	float mPhi_ = XM_PIDIV4;   // 45°
	float mRadius_ = 5.0f;   


	const aiScene* sponza;
	std::vector<Vertex> vertices;
	std::vector<std::uint32_t> indices;
};

#endif //DX12APP_