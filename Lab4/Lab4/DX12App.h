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
#include "texture.h"
#include "materials.h"
#include "submesh.h"
#include "g_buffer.h"
#include "rendering_system.h"
#include "light.h"
#include <unordered_map>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "camera.h"
#include <DirectXCollision.h>
#include "octree.h"
#include "particle.h"

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
	void LoadTextures();
	void CreateSRV();
	void CreateSamplerHeap();

	void OnResize();
	void SetViewport();
	void SetScissor();

	void CalculateGameStats(HWND hWnd);

	void Draw();
	void DrawToGBuffer(ComPtr<ID3D12GraphicsCommandList> m_command_list_);
	void DrawLights(ComPtr<ID3D12GraphicsCommandList> m_command_list_);
	void DrawToStreamOutput(ComPtr<ID3D12GraphicsCommandList> m_command_list_);
	void DrawWireframe(ComPtr<ID3D12GraphicsCommandList> m_command_list_);
	void DrawParticles(ComPtr<ID3D12GraphicsCommandList> m_command_list_);

	void FlushCommandQueue();

	void InitProjectionMatrix();
	void CreateVertexBuffer();
	void CreateIndexBuffer();

	void OnMouseDown(HWND hWnd);
	void OnMouseUp();

	void Update();

	void InitUploadBuffers();
	void CreateConstantBufferView();
	void CreateStructuredBuffersSRV();

	void CompileShaders();

	void CreateSOBuffers();

	void BuildBulbGeometry();
	void BuildBoxGeometry();
	void InitRenderSystem();
	void Parsing();
	void ParseFile(const std::string& filename, const Matrix& transform, UINT instanceCount);

	void ParseNode(const std::string& filename, aiNode* node, const aiScene* scene, const Matrix& transform, int materialOffset, 
		std::vector<Vertex>& vertices, std::vector<std::uint32_t>& indices, UINT instanceCount);

	void ParseMesh(const std::string& filename, const aiScene* scene, aiMesh* mesh, const Matrix& transform, int materialOffset, 
		std::vector<Vertex>& vertices, std::vector<std::uint32_t>& indices, UINT instanceCount);

	void ExtractMaterialData(const std::string& filename, int MaterialIndex, aiMaterial* material);

	ComPtr<ID3D12Device> GetDevice() const { return m_device_; }
	Camera& GetCamera() { return camera; }
	GameTimer& GetTimer() { return gt; }
	bool IsDeviceCreated() { return m_device_ != nullptr; }
	ComPtr<ID3D12GraphicsCommandList> GetCommandList() const { return m_command_list_; }

	void SetClientWH(int width, int height) {
		m_client_width_ = width;
		m_client_height_ = height;
	}

	bool m_key_states[256] = { false };
private:
	void EnableDebug();
	GameTimer gt;
	DXGI_FORMAT m_back_buffer_format_ = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT m_depth_stencil_format_ = DXGI_FORMAT_D24_UNORM_S8_UINT;
	int m_client_width_ = 800;
	int m_client_height_ = 600;
	ComPtr<IDXGIFactory4> m_dxgi_factory_ = nullptr;
	ComPtr<ID3D12Device> m_device_ = nullptr;
	ComPtr<ID3D12Fence> m_fence_;
	UINT64 m_current_fence_ = 0;
	UINT m_RTV_descriptor_size_ = 0;
	UINT m_DSV_descriptor_size_ = 0;
	UINT m_CbvSrvUav_descriptor_size_ = 0;
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels_;
	
	ComPtr<ID3D12CommandQueue> m_command_queue_ = nullptr;
	ComPtr<ID3D12CommandAllocator> m_direct_cmd_list_alloc_ = nullptr;
	ComPtr<ID3D12GraphicsCommandList> m_command_list_ = nullptr;
	
	ComPtr<IDXGISwapChain> m_swap_chain_ = nullptr;

	ComPtr<ID3D12DescriptorHeap> m_RTV_heap_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> m_DSV_heap_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> m_CBV_SRV_heap_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> m_sampler_heap = nullptr;
	int m_current_back_buffer_ = 0;

	ComPtr<ID3D12Resource> m_swap_chain_buffer_[2];
	ComPtr<ID3D12Resource> m_DSV_buffer = nullptr;

	D3D12_VIEWPORT vp_;
	D3D12_RECT m_scissor_rect_;

	ComPtr<ID3D12Resource> VertexBufferGPU_ = nullptr;
	ComPtr<ID3D12Resource> VertexBufferUploader_ = nullptr;
	D3D12_VERTEX_BUFFER_VIEW VertexBuffers[1];

	ComPtr<ID3D12Resource> IndexBufferGPU_ = nullptr;
	ComPtr<ID3D12Resource> IndexBufferUploader_ = nullptr;
	D3D12_INDEX_BUFFER_VIEW ibv;

	std::unique_ptr<UploadBuffer<ObjectConstants>> CBUploadBuffer = nullptr;
	std::unique_ptr<UploadBuffer<Matrices>> MatricesBuffer = nullptr;
	std::unique_ptr<UploadBuffer<MaterialConstants>> MaterialCB = nullptr;
	std::unique_ptr<UploadBuffer<LightConstants>> LightBuffer = nullptr;
	std::unique_ptr<UploadBuffer<CameraConstants>> CameraCB = nullptr;
	std::unique_ptr<UploadBuffer<HullBuffer>> HullCB = nullptr;
	std::unique_ptr<UploadBuffer<MeshInstanceData>> InstanceBuffer = nullptr;
	std::unique_ptr<UploadBuffer<WireframeInstanceData>> WireframeInstanceBuffer = nullptr;
	std::unique_ptr<UploadBuffer<Particle>> ParticleBuffer = nullptr;

	UINT PARTICLE_COUNT = 10000;
	std::vector<Particle> particles;


	std::vector<MeshInstanceData> instances;

	POINT m_mouse_last_pos_;
	Camera camera;
	BVH octree;

	std::vector<UINT> visibleIndices;


	const aiScene* sponza;
	std::vector<Vertex> vertices;
	std::vector<std::uint32_t> indices;
	std::vector<UINT> MeshIndexCounts;
	std::unordered_map<std::wstring, std::unique_ptr<Texture>> mTextures;
	std::vector<aiMaterial*> mMaterials_;
	std::vector<int> mMeshesMaterialIndex;
	std::vector<MaterialConstants> materialData;
	std::vector<Submesh> mSubmeshes;
	Submesh SOMesh;
	ComPtr<ID3D12Resource> mDefaultTexture;
	std::vector<std::string> materialNames;

	RenderingSystem* renderSystem = nullptr;

	ComPtr<ID3D12Resource> mSphereVB = nullptr;
	ComPtr<ID3D12Resource> mSphereIB = nullptr;
	D3D12_VERTEX_BUFFER_VIEW mSphereVbv;
	D3D12_INDEX_BUFFER_VIEW mSphereIbv;
	UINT mSphereIndexCount = 0;

	ComPtr<ID3D12Resource> wireframeVB = nullptr;
	ComPtr<ID3D12Resource> wireframeIB = nullptr;
	D3D12_VERTEX_BUFFER_VIEW wireframeVBV;
	D3D12_INDEX_BUFFER_VIEW wireframeIBV;

	ComPtr<ID3D12Resource> SOBuffer_ = nullptr;
	ComPtr<ID3D12Resource> filledSizeBuffer_ = nullptr;
	ComPtr<ID3D12Resource> readbackBuffer_ = nullptr;
	D3D12_STREAM_OUTPUT_BUFFER_VIEW SOView_ = {};
	bool isFirstFrame = true;
};

#endif //DX12APP_