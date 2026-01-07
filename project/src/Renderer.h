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
#include "Vector2.h"
#include "ColorRGB.h"

#include "Camera.h"
#include "Matrix.h"

namespace dae
{
	class Texture;
	class Effect {
		ID3DX11Effect* m_pEffect;
		ID3DX11EffectTechnique* m_pTechnique;
		ID3D11InputLayout* m_pInputLayout;

		ID3DX11EffectMatrixVariable* m_pWorldViewProjection;
		ID3DX11EffectMatrixVariable* m_pWorld;

		ID3DX11EffectVectorVariable* m_pLightDirection;
		ID3DX11EffectVectorVariable* m_pCameraOrigin;
		ID3DX11EffectVectorVariable* m_pSampleType;

		ID3DX11EffectShaderResourceVariable* m_pDiffuseMapVariable;
		ID3DX11EffectShaderResourceVariable* m_pSpecularMapVariable;
		ID3DX11EffectShaderResourceVariable* m_pNormalMapVariable;
		ID3DX11EffectShaderResourceVariable* m_pGlossMapVariable;

		ID3DX11EffectSamplerVariable* m_pSamplerState;

		static ID3DX11Effect* LoadEffect(ID3D11Device* pDevice, const std::wstring assetFile);

	public:
		explicit Effect(ID3D11Device* pDevice, const std::wstring assetFile);
		~Effect();

		ID3D11InputLayout* GetInputLayout() const { return m_pInputLayout; }
		ID3DX11EffectTechnique* GetTechnique() const { return m_pTechnique; }

		void SetWorldViewProj(const Matrix& m);
		void SetWorld(const Matrix& m);
		void SetDiffuseMap(const Texture* diffuse);
		void SetSpecularMap(const Texture* diffuse);
		void SetNormalMap(const Texture* diffuse);
		void SetGlossMap(const Texture* diffuse);
		void SetCameraOrigin(const Vector3& v);
		void SetLightDirection(const Vector3& v);
		void SetSamplerState(ID3D11SamplerState* v);
	};

	struct Vertex {
		Vector3 Position;
		ColorRGB Color;
		Vector2 Uv{};
		Vector3 Normal{};
		Vector3 Tangent{};
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
		uint32_t GetNumIndices() const { return m_NumIndices; }
	};

	struct Container {
		Mesh* mesh;
		Effect* effect;
		Texture* diffuseMap;
		Texture* specularMap;
		Texture* normalMap;
		Texture* glossMap;
		Matrix world{ Matrix::CreateIdentity() };
	};

	class Scene {
		std::vector<Container> m_Containers{};
		std::vector<Mesh*> m_Meshes{};
		std::vector<Effect*> m_Effects{};
		std::vector<Texture*> m_Textures{};

		void AddEffect(Effect* effect);
		void AddMesh(Mesh* mesh);
		void AddContainer(Container& container);
		void AddTexture(Texture* texture);

	public:
		Scene();
		~Scene();

		void InitializeScene(ID3D11Device* pDevice);

		void GenerateMips(ID3D11DeviceContext* pDeviceContext);

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

		void Update(const Timer* pTimer, bool leftClick, bool rightClick);
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

		ID3D11SamplerState* m_pSamplerPoint;
		ID3D11SamplerState* m_pSamplerLinear;
		ID3D11SamplerState* m_pSamplerAntisotropic;

		Camera m_Camera{};

		Scene* m_pScene{};

		Vector3 m_LightDirection{ Vector3(0.577f,0.577f ,0.577f).Normalized() };

		int m_Width{};
		int m_Height{};

		bool m_IsInitialized{ false };

		//DIRECTX
		HRESULT InitializeDirectX(RendererInitResult& value);
		void InitializeSamplerStates();
		//...
	};
}
