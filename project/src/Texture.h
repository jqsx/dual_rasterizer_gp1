#pragma once

class ID3D11Texture2D;
class ID3D11Device;
class ID3D11ShaderResourceView;
class SDL_Surface;
#include <string>

namespace dae {
	class Texture final {
	private:
		ID3D11Texture2D* m_pResource;
		ID3D11ShaderResourceView* m_pResourceViewer;

		SDL_Surface* m_pSurface;
	public:
		Texture(ID3D11Device* pDevice, const std::string& pSurface);
		~Texture();

		ID3D11ShaderResourceView* GetSRV() const { return m_pResourceViewer; }
	};
}