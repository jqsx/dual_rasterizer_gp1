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
#include <string>

#include "DataTypes.h"

#include <vector>

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
		ID3DX11EffectRasterizerVariable* m_pRasterizerState;

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
		void SetRasterizerState(ID3D11RasterizerState* v);
	};

	class Mesh {
		ID3D11Buffer* m_pVertexBuffer;
		ID3D11Buffer* m_pIndexBuffer;

		uint32_t m_NumIndices;

		std::vector<Vertex> m_Vertices;
		std::vector<unsigned int> m_Indices;
	public:
		explicit Mesh(ID3D11Device* pDevice, const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
		~Mesh();

		ID3D11Buffer* GetVertexBuffer() const { return m_pVertexBuffer; }
		ID3D11Buffer* GetIndexBuffer() const { return m_pIndexBuffer; }
		uint32_t GetNumIndices() const { return m_NumIndices; }

		const std::vector<Vertex>& GetVertices() const { return m_Vertices; }
		const std::vector<unsigned int>& GetIndices() const { return m_Indices; }
	};

	struct Container {
		Mesh* mesh{};
		Effect* effect{};
		Texture* diffuseMap{};
		Texture* specularMap{};
		Texture* normalMap{};
		Texture* glossMap{};
		soft::PrimitiveTopology topology{ soft::PrimitiveTopology::TriangleList };
		float rotation{};
		Matrix world{ Matrix::CreateIdentity() };
		bool isFlame{ 0 };
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

		soft::RenderSettings renderSettings{};

		void InitializeScene(ID3D11Device* pDevice);

		void GenerateMips(ID3D11DeviceContext* pDeviceContext);

		const std::vector<Container>& GetContainers() const;
		std::vector<Container>& GetContainersM();
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

		void Update(const Timer* pTimer, bool leftClick, bool rightClick, bool useSoftwareRasterizer);
		void Render() const;

		void RenderSoftwareRasterizer();

		Scene* GetScene() const { return m_pScene; }

	private:
		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{};

		Vector2 m_Min{};
		Vector2 m_Max{};

		int m_Width{};
		int m_Height{};

		Int_AABB m_ScreenBounds{ {0, 0}, {0, 0} };

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

		ID3D11RasterizerState* m_pCullFront;
		ID3D11RasterizerState* m_pCullBack;
		ID3D11RasterizerState* m_pCullNone;

		Camera m_Camera{};

		const float clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
		const float clearColorHardware[4] = { 0.39f, 0.59f, 0.93f, 1.0f };
		const ColorRGB clearColorSoftware{ 0.39f, 0.39f, 0.39f };

		Scene* m_pScene{};

		Vector3 m_LightDirection{ Vector3(0.577f,-0.577f ,0.577f).Normalized() };

		bool m_IsInitialized{ false };

		//DIRECTX
		HRESULT InitializeDirectX(RendererInitResult& value);
		void InitializeSamplerStates();
		//...

		// Software rasterizer

		void VertexTransformationFunction(const Matrix& objectToWorld, const Matrix& modelViewProjection, const std::vector<Vertex>& vertices_in, std::vector<soft::Vertex_Out>& vertices_out) const;
		void VertexInformation(const Matrix& objectTOWorld, const Matrix& modelViewProjection, const Vertex& IN, soft::Vertex_Out& OUT) const;
		void DrawTriangle(const soft::Triangle& triangle, const Container& container);

		bool PixelShading(ColorRGB& fragColor, const soft::VS_OUT& in, float& alpha, bool isFlame) const;

		bool isPixelInTriangle(const Vector2& c0, const Vector2& c1, const Vector2& c2, const Vector2& pixel, bool isFlame, bool& isBackFace);
		Vector2 NormlPixelToScreen(const Vector2& normlPixel) const;

		static Vector2 center(int x, int y);
		void ClearBuffers() const;

		static Vector3 GetBarycentricCoord(const Vector2& c0, const Vector2& c1, const Vector2& c2, const Vector2& pixel);

		static Vector2 v_max(const Vector2& v1, const Vector2& v2, const Vector2& v3);

		static Vector2 v_min(const Vector2& v1, const Vector2& v2, const Vector2& v3);

		static Vector2 v_Clamp(const Vector2& v, const Vector2& min, const Vector2& max);

		static bool isClose(const Vector2& v0, const Vector2& p, float distance);

		static void MinMaxAABB(const soft::Triangle& screenSpace, Vector2& max, Vector2& min);

		Vector2 VectorRangeToPixelCoord(const Vector2& v) const;

		static Vector2 Ceil(const Vector2& v);

		static Vector2 Floor(const Vector2& v);
		Vector2 PixelCoordToScreenCoord(const Vector2& v) const;
		
		float GetObservableArea(const Vector3& normal) const;

		static ColorRGB GetLambertColor(const ColorRGB& cd, float kd);

		static Matrix GetTangentSpaceAxis(const Vector3& tangent, const Vector3& normal);

		static float Phong(const Vector3& l, const Vector3& n, const Vector3& v, float ks, float e);

		static Int2 vToi(const Vector2& v);
	};
}
