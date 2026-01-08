#include "Texture.h"
#include <d3d11.h>
#include <SDL.h>

#include <iostream>
#include "Utils.h"

using namespace dae;


Texture::Texture(ID3D11Device* pDevice, const std::string& path) : m_pResource{nullptr}, m_pResourceViewer{nullptr}, m_pSurfacePixels{nullptr}, m_pSurface{nullptr}
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

	m_pSurfacePixels = (uint32_t*)m_pSurface->pixels;
}

Texture::~Texture()
{	
	if (m_pSurface)
	{
		SDL_FreeSurface(m_pSurface);
		m_pSurfacePixels = nullptr;
		m_pSurface = nullptr;
	}
	_RELEASE_DX11_PTR(m_pResourceViewer)
	_RELEASE_DX11_PTR(m_pResource)
}

ColorRGB Texture::Sample(const Vector2& uv) const
{
	//TODO
	//Sample the correct texel for the given uv

	const int px = Clamp(static_cast<int>(uv.x * float(m_pSurface->w)), 0, m_pSurface->w - 1);
	const int py = Clamp(static_cast<int>(uv.y * float(m_pSurface->h)), 0, m_pSurface->h - 1);

	return SamplePixel(px, py);
}

ColorRGB Texture::SampleLinear(const Vector2& uv) const {
	const int px_l = Clamp(static_cast<int>(uv.x * float(m_pSurface->w)), 0, m_pSurface->w - 1);
	const int py_l = Clamp(static_cast<int>(uv.y * float(m_pSurface->h)), 0, m_pSurface->h - 1);

	const int px_h = Clamp(static_cast<int>(ceilf(uv.x * float(m_pSurface->w))), 0, m_pSurface->w - 1);
	const int py_h = Clamp(static_cast<int>(ceilf(uv.y * float(m_pSurface->h))), 0, m_pSurface->h - 1);

	return ColorRGB::Lerp(SamplePixel(px_l, py_l), SamplePixel(px_h, py_h), 0.5f);
}

ColorRGB Texture::SamplePixel(int px, int py) const {
	const int blueMask = 0xFF0000, greenMask = 0xFF00, redMask = 0xFF;

	uint32_t rgb = m_pSurfacePixels[px + py * m_pSurface->w];

	return { float((rgb & redMask)) / 255.f, float((rgb & greenMask) >> 8) / 255.f, float((rgb & blueMask) >> 16) / 255.f };
}

ColorRGBA dae::Texture::SampleRGBA(float x, float y) const
{
	const int px = Clamp(static_cast<int>(x * float(m_pSurface->w)), 0, m_pSurface->w - 1);
	const int py = Clamp(static_cast<int>(y * float(m_pSurface->h)), 0, m_pSurface->h - 1);

	const int blueMask = 0xFF0000, greenMask = 0xFF00, redMask = 0xFF, alphaMask = 0xFF000000;

	uint32_t rgb = m_pSurfacePixels[px + py * m_pSurface->w];

	return { float((rgb & redMask)) / 255.f, float((rgb & greenMask) >> 8) / 255.f, float((rgb & blueMask) >> 16) / 255.f, float((rgb & alphaMask) >> 24) / 255.0f};
}

Vector3 Texture::SampleNormal(const Vector2& uv) const {
	ColorRGB color = Sample(uv);
	return { color.r * 2.0f - 1.0f, color.g * 2.0f - 1.0f, (color.b - 0.5f) * 2.0f };
}