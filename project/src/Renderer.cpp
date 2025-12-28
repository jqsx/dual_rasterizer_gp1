//External includes
#include "SDL.h"
#include "SDL_surface.h"

// Standard includes
#include <iostream>

//Project includes
#include "Renderer.h"

using namespace dae;

#pragma region Renderer
Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Initialize DirectX pipeline
	RendererInitResult render_result{};
	const HRESULT result = InitializeDirectX(render_result);
	if (result == S_OK)
	{
		m_IsInitialized = true;
		std::cout << "DirectX is initialized and ready!\n";

		m_pScene = new Scene();
		m_pScene->InitializeScene(m_pDevice);
	}
	else
	{
		std::cout << "DirectX initialization failed!\n";
		std::cout << result << "\n";
		std::cout << render_result.stage << "\n";
	}
}

Renderer::~Renderer()
{
	delete m_pScene;

	_RELEASE(m_pRenderTargetView)
	_RELEASE(m_pRenderTargetBuffer)
	_RELEASE(m_pDepthStencilView)
	_RELEASE(m_pDepthStencilBuffer)
	_RELEASE(m_pDxgiSwapChain)

	if (m_pDeviceContext) {
		m_pDeviceContext->ClearState();
		m_pDeviceContext->Flush();
		m_pDeviceContext->Release();
		m_pDeviceContext = nullptr;
	}
	_RELEASE(m_pDevice)
	_RELEASE(m_pDxgiFactory)
}

void Renderer::Update(const Timer* pTimer)
{

}


void Renderer::Render() const
{
	if (!m_IsInitialized)
		return;

	const float clearColor[4] = { 0.1f, 0.1f, 0.3f, 1.0f };

	m_pDeviceContext->ClearRenderTargetView(
		m_pRenderTargetView,
		clearColor
	);

	m_pDeviceContext->ClearDepthStencilView(
		m_pDepthStencilView,
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1.0f,
		0
	);
	
	m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (const Container& container : m_pScene->GetContainers()) {
		m_pDeviceContext->IASetInputLayout(container.effect->GetInputLayout());

		constexpr UINT stride = sizeof(Vertex);
		constexpr UINT offset = 0;

		ID3D11Buffer* vertex_buffer = container.mesh->GetVertexBuffer();

		m_pDeviceContext->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, &offset);
		m_pDeviceContext->IASetIndexBuffer(container.mesh->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

		D3DX11_TECHNIQUE_DESC techdesc{};
		container.effect->GetTechnique()->GetDesc(&techdesc);
		for (UINT p = 0; p < techdesc.Passes; p++) {
			container.effect->GetTechnique()->GetPassByIndex(p)->Apply(0, m_pDeviceContext);
			m_pDeviceContext->DrawIndexed(container.mesh->GetNumIndices(), 0, 0);
		}
	}

	m_pDxgiSwapChain->Present(0, 0);
}

HRESULT Renderer::InitializeDirectX(RendererInitResult& value)
{
	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;

	uint32_t createDeviceFlags = 0;

#if defined(DEBUG) || defined(_DEBUG)
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	HRESULT result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, createDeviceFlags, &featureLevel, 1, D3D11_SDK_VERSION, &m_pDevice, nullptr, &m_pDeviceContext);
	value.stage = "CREATEDEVICE";
	if (FAILED(result)) {
		return result;
	}

	// Create DXGI Factory
	result = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&m_pDxgiFactory));
	value.stage = "CREATEFACTORY";
	if (FAILED(result))
		return result;

	// Create Swap Chain
	DXGI_SWAP_CHAIN_DESC swapChainDesc{};
	swapChainDesc.BufferDesc.Width = m_Width;
	swapChainDesc.BufferDesc.Height = m_Height;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 1;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 1;
	swapChainDesc.Windowed = true;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags = 0;

	// Get HWND window for swap chain
	SDL_SysWMinfo sysWMinfo{};
	SDL_GetVersion(&sysWMinfo.version);
	SDL_GetWindowWMInfo(m_pWindow, &sysWMinfo);
	swapChainDesc.OutputWindow = sysWMinfo.info.win.window;

	result = m_pDxgiFactory->CreateSwapChain(m_pDevice, &swapChainDesc, &m_pDxgiSwapChain);
	value.stage = "CREATESWAPCHAIN";
	if (FAILED(result))
		return result;

	// Create depth buffer
	D3D11_TEXTURE2D_DESC depthStencilDesc{};
	depthStencilDesc.Width = m_Width;
	depthStencilDesc.Height = m_Height;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D10_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	// Depth stencil
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
	depthStencilViewDesc.Format = depthStencilDesc.Format;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;

	// Init depth buffer
	result = m_pDevice->CreateTexture2D(&depthStencilDesc, nullptr, &m_pDepthStencilBuffer);
	value.stage = "CREATEDEPTHBUFFER";
	if (FAILED(result))
		return result;

	// Init depth stencil
	result = m_pDevice->CreateDepthStencilView(m_pDepthStencilBuffer, &depthStencilViewDesc, &m_pDepthStencilView);
	value.stage = "CREATEDEPTHSTENCIL";
	if (FAILED(result))
		return result;

	// Init color buffer
	result = m_pDxgiSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&m_pRenderTargetBuffer));
	value.stage = "CREATECOLORBUFFER";
	if (FAILED(result))
		return result;

	// Init color view
	result = m_pDevice->CreateRenderTargetView(m_pRenderTargetBuffer, nullptr, &m_pRenderTargetView);
	value.stage = "CREATECOLORTARGET";
	if (FAILED(result))
		return result;

	// Output merger stage
	m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);

	D3D11_VIEWPORT viewport{};
	viewport.Width = m_Width;
	viewport.Height = m_Height;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	m_pDeviceContext->RSSetViewports(1, &viewport);

	return S_OK;
}

#pragma endregion Renderer

#pragma region Effect
ID3DX11Effect* dae::Effect::LoadEffect(ID3D11Device* pDevice, const std::wstring assetFile) {
	HRESULT result;
	ID3D10Blob* pErrorBlob{ nullptr };
	ID3DX11Effect* pEffect;

	DWORD shaderFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	shaderFlags |= D3DCOMPILE_DEBUG;
	shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	result = D3DX11CompileEffectFromFile(assetFile.c_str(), nullptr, nullptr, shaderFlags, 0, pDevice, &pEffect, &pErrorBlob);
	if (FAILED(result)) {
		if (pErrorBlob != nullptr) {
			const char* pErrors = static_cast<char*>(pErrorBlob->GetBufferPointer());

			for (unsigned int i = 0; i < pErrorBlob->GetBufferSize(); i++) {
				std::cout << pErrors[i];
			}

			pErrorBlob->Release();
			pErrorBlob = nullptr;
		}
		else {
			std::wcout << "EffectLoader: Failed to CreateEffectFromFile!\nPath: " << assetFile << std::endl;
			if (pEffect != nullptr) {
				pEffect->Release();
				pEffect = nullptr;
			}
			return nullptr;
		}
	}

	return pEffect;
}

dae::Effect::Effect(ID3D11Device* pDevice, const std::wstring assetFile) : m_pEffect{ nullptr }, m_pInputLayout{ nullptr }, m_pTechnique{ nullptr }
{
	m_pEffect = LoadEffect(pDevice, assetFile); // Logs errors
	if (m_pEffect != nullptr) { // Avoids accessing nullptr
		m_pTechnique = m_pEffect->GetTechniqueByName("DefaultTechnique");

		// Vertex Layout
		static constexpr uint32_t numElements{ 2 };
		D3D11_INPUT_ELEMENT_DESC vertexDesc[numElements]{};

		vertexDesc[0].SemanticName = "POSITION";
		vertexDesc[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		vertexDesc[0].AlignedByteOffset = 0;
		vertexDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

		vertexDesc[1].SemanticName = "COLOR";
		vertexDesc[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		vertexDesc[1].AlignedByteOffset = 12;
		vertexDesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

		// Input Layout
		D3DX11_PASS_DESC passDesc{};
		m_pTechnique->GetPassByIndex(0)->GetDesc(&passDesc);

		const HRESULT result = pDevice->CreateInputLayout(vertexDesc, numElements, passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &m_pInputLayout);
		if (FAILED(result))
			return;
	}
}

dae::Effect::~Effect()
{
	if (m_pInputLayout != nullptr) {
		m_pInputLayout->Release();
		m_pInputLayout = nullptr;
	}
	if (m_pTechnique != nullptr) {
		m_pTechnique->Release();
		m_pTechnique = nullptr;
	}
	if (m_pEffect != nullptr) {
		m_pEffect->Release();
		m_pEffect = nullptr;
	}
}

#pragma endregion Effect

#pragma region Mesh
dae::Mesh::Mesh(ID3D11Device* pDevice, const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) : m_NumIndices{ 0 }, m_pIndexBuffer{ nullptr }, m_pVertexBuffer{ nullptr }
{
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.ByteWidth = sizeof(Vertex) * static_cast<uint32_t>(vertices.size());
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = vertices.data();

	HRESULT result = pDevice->CreateBuffer(&bd, &initData, &m_pVertexBuffer);
	if (FAILED(result))
	{
		std::cout << "Failed to initialize vertex buffer." << std::endl;
		return;
	}

	m_NumIndices = static_cast<uint32_t>(indices.size());
	bd.ByteWidth = sizeof(uint32_t) * m_NumIndices;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;

	initData.pSysMem = indices.data();
	result = pDevice->CreateBuffer(&bd, &initData, &m_pIndexBuffer);
	if (FAILED(result)) {
		std::cout << "Failed to initialize index buffer." << std::endl;
		return;
	}

	std::cout << "Successfully created mesh." << std::endl;
}

dae::Mesh::~Mesh()
{
	if (m_pVertexBuffer != nullptr) {
		m_pVertexBuffer->Release();
		m_pVertexBuffer = nullptr;
	}
	if (m_pIndexBuffer != nullptr) {
		m_pIndexBuffer->Release();
		m_pIndexBuffer = nullptr;
	}
}

#pragma endregion Mesh

#pragma region Scene

void dae::Scene::AddEffect(Effect* effect)
{
	m_Effects.emplace_back(effect);
}

void dae::Scene::AddMesh(Mesh* mesh)
{
	m_Meshes.emplace_back(mesh);
}

void dae::Scene::AddContainer(Container& container)
{
	m_Containers.emplace_back(container);
}

dae::Scene::Scene()
{

}

dae::Scene::~Scene()
{
	m_Containers.clear();
	for (Mesh* mesh : m_Meshes) {
		delete mesh;
	}
	m_Meshes.clear();
	for (Effect* effect : m_Effects) {
		delete effect;
	}
	m_Effects.clear();
}

void dae::Scene::InitializeScene(ID3D11Device* pDevice)
{
	std::vector<dae::Vertex> vertices{
		{ { 0.f, 0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f } },
		{ { 0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } },
		{ { -0.5f, -0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f } }
	};
	std::vector<uint32_t> indices{ 0, 1, 2 };
	Mesh* helloTriangle = new dae::Mesh(pDevice, vertices, indices);

	AddMesh(helloTriangle);

	Effect* posColEffect = new dae::Effect(pDevice, L"./resources/PosCol3D.fx");

	AddEffect(posColEffect);

	Container container{};

	container.effect = posColEffect;
	container.mesh = helloTriangle;

	AddContainer(container);
}

#pragma endregion Scene