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
#include "Vector3.h"
#include "ColorRGB.h"

namespace dae
{
	class Effect {
		ID3DX11Effect* m_pEffect;
		ID3DX11EffectTechnique* m_pTechnique;
		ID3D11InputLayout* m_pInputLayout;

		static ID3DX11Effect* LoadEffect(ID3D11Device* pDevice, const std::wstring assetFile);

	public:
		explicit Effect(ID3D11Device* pDevice, const std::wstring assetFile);
		~Effect();

		ID3D11InputLayout* GetInputLayout() const { return m_pInputLayout; }
		ID3DX11EffectTechnique* GetTechnique() const { return m_pTechnique; }
	};

	struct Vertex {
		Vector3 Position;
		ColorRGB Color;
	};

	class Mesh {
		ID3D11Buffer* m_pVertexBuffer;
		ID3D11Buffer* m_pIndexBuffer;

		uint32_t m_NumIndices;
	public:
		explicit Mesh(ID3D11Device* pDevice, const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
		~Mesh();

		ID3D11Buffer* GetVertexBuffer() const { return m_pVertexBuffer; }
		ID3D11Buffer* GetIndexBuffer() const { return m_pIndexBuffer; }
	};

	struct Container {
		Mesh* mesh;
		Effect* effect;
	};

	class Scene {
		std::vector<Container> m_Containers{};
		std::vector<Mesh*> m_Meshes{};
		std::vector<Effect*> m_Effects{};

		void AddEffect(Effect* effect);
		void AddMesh(Mesh* mesh);
		void AddContainer(Container& container);

	public:
		Scene();
		~Scene();

		void InitializeScene(ID3D11Device* pDevice);

		const std::vector<Container>& GetContainers() const { return m_Containers; };
	};

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

		Scene* m_pScene{};

		int m_Width{};
		int m_Height{};

		bool m_IsInitialized{ false };

		//DIRECTX
		HRESULT InitializeDirectX(RendererInitResult& value);

#define _RELEASE(resource_ptr) if (resource_ptr != nullptr) { resource_ptr->Release(); resource_ptr = nullptr; }
		//...
	};
}
