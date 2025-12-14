#pragma once

// SDL Headers
#include "SDL.h"
#include "SDL_syswm.h"
#include "SDL_surface.h"
#include "SDL_image.h"

// DirectX Headers
#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11effect.h>

// Framework Headers
#include "Timer.h"

namespace dae
{
	class Mesh;
	class Effect;

	struct RendererInitResult {
		std::string stage;
	};

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(const Timer* pTimer);
		void Render() const;

	private:
		SDL_Window* m_pWindow{};

		ID3D11Device* m_pDevice;
		ID3D11DeviceContext* m_pDeviceContext;
		IDXGIFactory1* m_pDxgiFactory{};
		IDXGISwapChain* m_pDxgiSwapChain{};

		// Depth buffer
		ID3D11Texture2D* m_pDepthStencilBuffer{};
		ID3D11DepthStencilView* m_pDepthStencilView{};

		// Color buffer
		ID3D11Texture2D* m_pRenderTargetBuffer{};
		ID3D11RenderTargetView* m_pRenderTargetView{};

		int m_Width{};
		int m_Height{};

		bool m_IsInitialized{ false };

		//DIRECTX
		HRESULT InitializeDirectX(RendererInitResult& value);

		void RenderUsing(const Mesh* pMesh, const Effect* pEffect) const;

#define _RELEASE(resource_ptr) if (resource_ptr != nullptr) { resource_ptr->Release(); resource_ptr = nullptr; }

		//...
	};
}
