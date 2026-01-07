#include "Texture.h"
#include <d3d11.h>
#include <d3dx11effect.h>
#include <SDL.h>

#include <iostream>
#include "Utils.h"

using namespace dae;


Texture::Texture(ID3D11Device* pDevice, const std::string& path)
{
	SDL_Surface* pSurface = m_pSurface = IMG_Load(path.c_str());

	if (!pSurface) {
		std::cerr << "Failed to load texture.\n";
		return;
	}

	pSurface = SDL_ConvertSurfaceFormat(m_pSurface, SDL_PIXELFORMAT_RGBA32, 0);

	SDL_FreeSurface(m_pSurface);
	if (!pSurface) {
		std::cerr << "Failed to convert surface to RGBA32\n";
		SDL_FreeSurface(pSurface);
		return;
	}
	m_pSurface = pSurface;

	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
	D3D11_TEXTURE2D_DESC desc{};

	desc.Width = pSurface->w;
	desc.Height = pSurface->h;

	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = pSurface->pixels;
	initData.SysMemPitch = static_cast<UINT>(pSurface->pitch);
	initData.SysMemSlicePitch = static_cast<UINT>(pSurface->h * pSurface->pitch);

	HRESULT result = pDevice->CreateTexture2D(&desc, &initData, &m_pResource);

	if (FAILED(result)) {
		std::cerr << "Failed to initialize D3D11 texture resource." << std::endl;
		return;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};

	SRVDesc.Format = format;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MipLevels = 1;

	result = pDevice->CreateShaderResourceView(m_pResource, &SRVDesc, &m_pResourceViewer);

	if (FAILED(result)) {
		std::cerr << "Failed to initialize D3D11 texture shader resource viewer." << std::endl;
		return;
	}
}

Texture::~Texture()
{	if (m_pSurface)
		SDL_FreeSurface(m_pSurface);
	_RELEASE_DX11_PTR(m_pResourceViewer)
	_RELEASE_DX11_PTR(m_pResource)
}
