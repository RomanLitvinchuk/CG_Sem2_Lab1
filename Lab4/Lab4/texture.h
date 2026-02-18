#ifndef TEXTURE_H_
#define TEXTURE_H_
#include <string>
#include <d3d12.h>
#include <wrl.h>

using namespace Microsoft::WRL;

struct Texture
{
	std::string name_;
	std::wstring filepath;
	ComPtr<ID3D12Resource> Resource = nullptr;
	ComPtr<ID3D12Resource> UploadHeap = nullptr;
};


#endif TEXTURE_H_
