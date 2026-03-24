#include "rendering_system.h"
#include "throw_if_failed.h"
#include "d3dUtil.h"
#include <iostream>
#include <random>

void RenderingSystem::CreateOpaqueRS(ComPtr<ID3D12Device> device) {
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	CD3DX12_DESCRIPTOR_RANGE srvTable[2];
	srvTable[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	srvTable[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
	CD3DX12_DESCRIPTOR_RANGE samplerTable;
	samplerTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable, D3D12_SHADER_VISIBILITY_ALL);
	slotRootParameter[1].InitAsDescriptorTable(2, srvTable, D3D12_SHADER_VISIBILITY_ALL);
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
	DWORD fileAttr = GetFileAttributes(L"shaders/opaque.hlsl");
	if (fileAttr == INVALID_FILE_ATTRIBUTES) {
		std::wcout << L"ERROR: Shader file not found!" << std::endl;
		MessageBox(NULL, L"Shader file not found!", L"Error", MB_OK);
		return;
	}
	opaqueVS_ = d3dUtil::CompileShader(L"shaders/opaque.hlsl", nullptr, "VS", "vs_5_0");
	opaquePS_ = d3dUtil::CompileShader(L"shaders/opaque.hlsl", nullptr, "PS", "ps_5_0");

	fileAttr = GetFileAttributes(L"shaders/light.hlsl");
	if (fileAttr == INVALID_FILE_ATTRIBUTES) {
		std::wcout << L"ERROR: Shader light.hlsl not found!" << std::endl;
		MessageBox(NULL, L"Shader light.hlsl not found!", L"Error", MB_OK);
		return;
	}
	lightVS_ = d3dUtil::CompileShader(L"shaders/light.hlsl", nullptr, "VS_FullScreenTriangle", "vs_5_0");
	lightPS_ = d3dUtil::CompileShader(L"shaders/light.hlsl", nullptr, "PS_DeferredLighting", "ps_5_0");

	fileAttr = GetFileAttributes(L"shaders/bulb.hlsl");
	if (fileAttr == INVALID_FILE_ATTRIBUTES) {
		std::wcout << L"ERROR: Bulb.hlsl not found" << std::endl;
		MessageBox(NULL, L"Shader bulb.hlsl not found", L"Error", MB_OK);
		return;
	}
	bulbVS_ = d3dUtil::CompileShader(L"shaders/bulb.hlsl", nullptr, "VS", "vs_5_0");
	bulbPS_ = d3dUtil::CompileShader(L"shaders/bulb.hlsl", nullptr, "PS", "ps_5_0");
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

void RenderingSystem::CreateLightRS(ComPtr<ID3D12Device> device) {
	CD3DX12_ROOT_PARAMETER rootParameter[4];
	CD3DX12_DESCRIPTOR_RANGE srvTable[4];
	srvTable[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	srvTable[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
	srvTable[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);
	srvTable[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3);
	CD3DX12_DESCRIPTOR_RANGE samplerTable;
	samplerTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
	rootParameter[0].InitAsConstantBufferView(0);
	rootParameter[1].InitAsConstants(1, 1);
	rootParameter[2].InitAsDescriptorTable(4, srvTable, D3D12_SHADER_VISIBILITY_ALL);
	rootParameter[3].InitAsDescriptorTable(1, &samplerTable, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_ROOT_SIGNATURE_DESC rootSignDesc(4, rootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&rootSignDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr) {
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);
	ThrowIfFailed(device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&lightRS_)));
}

void RenderingSystem::CreateLightPSO(ComPtr<ID3D12Device> device, std::vector<D3D12_INPUT_ELEMENT_DESC>& layout) {
	D3D12_RENDER_TARGET_BLEND_DESC RTBDesc;
	RTBDesc.BlendEnable = true;
	RTBDesc.LogicOpEnable = false;

	RTBDesc.SrcBlend = D3D12_BLEND_ONE;
	RTBDesc.DestBlend = D3D12_BLEND_ONE;
	RTBDesc.BlendOp = D3D12_BLEND_OP_ADD;

	RTBDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	RTBDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	RTBDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	RTBDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	D3D12_BLEND_DESC blendDesc;
	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.IndependentBlendEnable = false;
	blendDesc.RenderTarget[0] = RTBDesc;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { nullptr, 0 };
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	psoDesc.pRootSignature = lightRS_.Get();
	psoDesc.VS = { reinterpret_cast<BYTE*>(lightVS_->GetBufferPointer()), lightVS_->GetBufferSize() };
	psoDesc.PS = { reinterpret_cast<BYTE*>(lightPS_->GetBufferPointer()), lightPS_->GetBufferSize() };
	CD3DX12_RASTERIZER_DESC rastDesc(D3D12_DEFAULT);
	psoDesc.RasterizerState = rastDesc;
	psoDesc.BlendState = blendDesc;
	psoDesc.DepthStencilState.DepthEnable = false;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&lightPSO_)));

}

void RenderingSystem::GenerateTreeLights(std::vector<LightConstants>& lightsArray, Vector3 treeBasePosition, float treeHeight, float treeBaseRadius, int count) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dis(0.0f, 1.0f);

	for (int i = 0; i < count; ++i) {
		float u = dis(gen);
		float v = dis(gen);

		float h = treeHeight * (1.0f - std::sqrt(u)) + 50.0f;
		float r = treeBaseRadius * std::sqrt(u);
		float theta = 2.0f * XM_PI * v;

		Vector3 localPos(
			r * std::cos(theta),
			h,
			r * std::sin(theta)
		);

		LightConstants bulb = {};
		bulb.lightType = 1; // Point light
		bulb.lightPosition = treeBasePosition + localPos;
		bulb.lightRange = 100.0f;

		bulb.lightColor = { dis(gen), dis(gen), dis(gen) };

		lightsArray.push_back(bulb);
	}
}

void RenderingSystem::CreateBulbRS(ComPtr<ID3D12Device> device) {
	CD3DX12_ROOT_PARAMETER rootParameter[2];
	CD3DX12_DESCRIPTOR_RANGE srvTable;
	srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	rootParameter[0].InitAsConstantBufferView(0);
	rootParameter[1].InitAsDescriptorTable(1, &srvTable, D3D12_SHADER_VISIBILITY_ALL);

	CD3DX12_ROOT_SIGNATURE_DESC rootSignDesc(2, rootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&rootSignDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr) {
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);
	ThrowIfFailed(device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&bulbRS_)));
}

void RenderingSystem::CreateBulbPSO(ComPtr<ID3D12Device> device, std::vector<D3D12_INPUT_ELEMENT_DESC>& layout) {
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { layout.data(), (UINT)layout.size() };
	psoDesc.pRootSignature = bulbRS_.Get(); 

	psoDesc.VS = { reinterpret_cast<BYTE*>(bulbVS_->GetBufferPointer()), bulbVS_->GetBufferSize() };
	psoDesc.PS = { reinterpret_cast<BYTE*>(bulbPS_->GetBufferPointer()), bulbPS_->GetBufferSize() };

	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	psoDesc.DepthStencilState.DepthEnable = true;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;

	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&bulbPSO_)));
}