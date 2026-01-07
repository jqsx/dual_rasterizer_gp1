#pragma once

class ID3D11Texture2D;
class ID3D11Device;
class ID3D11ShaderResourceView;
class SDL_Surface;
#include <string>
#include "Vector2.h"
#include "Vector3.h"
#include "ColorRGB.h"

namespace dae {

	struct ColorRGBA {
		float r, g, b, a;
	};

	class Texture final {
	private:
		ID3D11Texture2D* m_pResource;
		ID3D11ShaderResourceView* m_pResourceViewer;

		SDL_Surface* m_pSurface;
		uint32_t* m_pSurfacePixels;
	public:
		Texture(ID3D11Device* pDevice, const std::string& pSurface);
		~Texture();

		ID3D11ShaderResourceView* GetSRV() const { return m_pResourceViewer; }

		ColorRGB Sample(const Vector2& uv) const;
		ColorRGB SampleLinear(const Vector2& uv) const;
		ColorRGB SamplePixel(int x, int y) const;
		ColorRGBA SampleRGBA(float x, float y) const;

		Vector3 SampleNormal(const Vector2& uv) const;
	};
}