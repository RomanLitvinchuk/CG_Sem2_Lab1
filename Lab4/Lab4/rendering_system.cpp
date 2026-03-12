#include "rendering_system.h"
#include "throw_if_failed.h"
#include "d3dUtil.h"
#include <iostream>

void RenderingSystem::CreateOpaqueRS(ComPtr<ID3D12Device> device) {
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	CD3DX12_DESCRIPTOR_RANGE srvTable;
	srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	CD3DX12_DESCRIPTOR_RANGE samplerTable;
	samplerTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable, D3D12_SHADER_VISIBILITY_ALL);
	slotRootParameter[1].InitAsDescriptorTable(1, &srvTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[2].InitAsDescriptorTable(1, &samplerTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[3].InitAsConstantBufferView(1);;

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		4, slotRootParameter,
		0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(
		&rootSigDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(),
		errorBlob.GetAddressOf()
	);

	if (errorBlob != nullptr) {
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}

	ThrowIfFailed(hr);
	ThrowIfFailed(device->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&opaqueRS_)
	));
}

void RenderingSystem::CompileShaders() {
	DWORD fileAttr = GetFileAttributes(L"opaque.hlsl");
	if (fileAttr == INVALID_FILE_ATTRIBUTES) {
		std::wcout << L"ERROR: Shader file not found!" << std::endl;
		MessageBox(NULL, L"Shader file not found!", L"Error", MB_OK);
		return;
	}
	opaqueVS_ = d3dUtil::CompileShader(L"opaque.hlsl", nullptr, "VS", "vs_5_0");
	std::cout << "Vertex shader are compiled" << std::endl;
	opaquePS_ = d3dUtil::CompileShader(L"opaque.hlsl", nullptr, "PS", "ps_5_0");
	std::cout << "Pixel shader are compiled" << std::endl;
}

void RenderingSystem::CreateOpaquePSO(ComPtr<ID3D12Device> device, std::vector<D3D12_INPUT_ELEMENT_DESC>& layout) {
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { layout.data(), (UINT)layout.size() };
	psoDesc.pRootSignature = opaqueRS_.Get();
	psoDesc.VS = { reinterpret_cast<BYTE*>(opaqueVS_->GetBufferPointer()), opaqueVS_->GetBufferSize() };
	psoDesc.PS = { reinterpret_cast<BYTE*>(opaquePS_->GetBufferPointer()), opaquePS_->GetBufferSize() };
	CD3DX12_RASTERIZER_DESC rastDesc(D3D12_DEFAULT);
	//rastDesc.CullMode = D3D12_CULL_MODE_NONE;
	rastDesc.FrontCounterClockwise = true;
	psoDesc.RasterizerState = rastDesc;
	//psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 2;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.RTVFormats[1] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&opaquePSO_)));
}