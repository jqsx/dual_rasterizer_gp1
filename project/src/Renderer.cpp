//External includes
#include "SDL.h"
#include "SDL_surface.h"

// Standard includes
#include <iostream>

#include "Utils.h"

#include "Texture.h"

//Project includes
#include "Renderer.h"
#include "DataTypes.h"

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

		m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
		m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
		m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

		m_pDepthBufferPixels = new float[m_Width * m_Height];

		InitializeSamplerStates();

		m_pScene = new Scene();
		m_Camera.Initialize(45.f, { 0.0f, 0.0f, -100.0f });
		m_pScene->InitializeScene(m_pDevice);
		m_pScene->GenerateMips(m_pDeviceContext);
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

	SDL_FreeSurface(m_pBackBuffer);
	delete[] m_pDepthBufferPixels;

	_RELEASE_DX11_PTR(m_pSamplerLinear)
	_RELEASE_DX11_PTR(m_pSamplerPoint)
	_RELEASE_DX11_PTR(m_pSamplerAntisotropic)

	_RELEASE_DX11_PTR(m_pRenderTargetView)
		
	_RELEASE_DX11_PTR(m_pRenderTargetBuffer)
	_RELEASE_DX11_PTR(m_pDepthStencilView)
	_RELEASE_DX11_PTR(m_pDepthStencilBuffer)
	_RELEASE_DX11_PTR(m_pDxgiSwapChain)

	if (m_pDeviceContext) {
		m_pDeviceContext->ClearState();
		m_pDeviceContext->Flush();
		m_pDeviceContext->Release();
		m_pDeviceContext = nullptr;
	}
	_RELEASE_DX11_PTR(m_pDevice)
	_RELEASE_DX11_PTR(m_pDxgiFactory)
}

void Renderer::Update(const Timer* pTimer, bool leftClick, bool rightClick, bool useSoftwareRasterizer)
{
	m_Camera.aspect = float(m_Width) / float(m_Height);
	m_Camera.Update(pTimer, leftClick, rightClick, useSoftwareRasterizer);

	if (m_pScene->renderSettings.hasRotation)
	{
		const Matrix worldMatrix = Matrix::CreateRotation(0.0f, pTimer->GetTotal() * M_PI / 4.0f, 0.0f) * Matrix::CreateTranslation({ 0.0f, 0.0f, 50.0f });
		for (Container& container : m_pScene->GetContainersM()) {
			container.world = worldMatrix;
		}
	}
}


void Renderer::Render() const
{
	if (!m_IsInitialized)
		return;

	m_pDeviceContext->ClearRenderTargetView(
		m_pRenderTargetView,
		m_pScene->renderSettings.useUniformClearColor ? clearColor : clearColorHardware
	);

	m_pDeviceContext->ClearDepthStencilView(
		m_pDepthStencilView,
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1.0f,
		0
	);
	
	m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (const Container& container : m_pScene->GetContainers()) {
		if (container.isFlame && !m_pScene->renderSettings.drawFireFx)
			continue;
		m_pDeviceContext->IASetInputLayout(container.effect->GetInputLayout());

		constexpr UINT stride = sizeof(Vertex);
		constexpr UINT offset = 0;

		ID3D11Buffer* vertex_buffer = container.mesh->GetVertexBuffer();

		m_pDeviceContext->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, &offset);
		m_pDeviceContext->IASetIndexBuffer(container.mesh->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

		Matrix projView = container.world * m_Camera.viewMatrix * m_Camera.projectionMatrix;

		//std::cout << std::endl;

		//for (size_t i = 0; i < 4; i++)
		//{
		//	std::cout << "[ ";
		//	for (size_t j = 0; j < 4; j++)
		//	{
		//		std::cout << projView[i][j] << ", ";
		//	}
		//	std::cout << " ] " << std::endl;
		//}

		container.effect->SetWorldViewProj(projView);
		container.effect->SetWorld(container.world);
		container.effect->SetDiffuseMap(container.diffuseMap);
		container.effect->SetNormalMap(container.normalMap);
		container.effect->SetSpecularMap(container.specularMap);
		container.effect->SetGlossMap(container.glossMap);
		container.effect->SetCameraOrigin(m_Camera.origin);
		container.effect->SetLightDirection(m_LightDirection);
		switch (m_pScene->renderSettings.samplingState) {
			case soft::RenderSettings::SamplingState::Point:
				container.effect->SetSamplerState(m_pSamplerPoint);
				break;
			case soft::RenderSettings::SamplingState::Linear:
				container.effect->SetSamplerState(m_pSamplerLinear);
				break;
			case soft::RenderSettings::SamplingState::Anisotropic:
				container.effect->SetSamplerState(m_pSamplerAntisotropic);
				break;
		}
		switch (m_pScene->renderSettings.cullMode) {
			case soft::RenderSettings::Back:
				container.effect->SetRasterizerState(m_pCullBack);
				break;
			case soft::RenderSettings::Front:
				container.effect->SetRasterizerState(m_pCullFront);
				break;
			case soft::RenderSettings::None:
				container.effect->SetRasterizerState(m_pCullNone);
				break;
		}

		D3DX11_TECHNIQUE_DESC techdesc{};
		container.effect->GetTechnique()->GetDesc(&techdesc);
		for (UINT p = 0; p < techdesc.Passes; p++) {
			container.effect->GetTechnique()->GetPassByIndex(p)->Apply(0, m_pDeviceContext);
			m_pDeviceContext->DrawIndexed(container.mesh->GetNumIndices(), 0, 0);
		}
	}

	m_pDxgiSwapChain->Present(0, 0);
}

void dae::Renderer::RenderSoftwareRasterizer()
{
	SDL_LockSurface(m_pBackBuffer);

	m_ScreenBounds.size.x = m_Width;
	m_ScreenBounds.size.y = m_Height;

	m_Camera.aspect = float(m_Width) / float(m_Height);

	ClearBuffers();

	soft::Triangle triangle;

	std::vector<soft::Vertex_Out> transformedVertices{};

	for (const Container& container : m_pScene->GetContainers()) {
		const Matrix flipYWorld = container.world * Matrix::CreateScale({ 1.0f, -1.0f, 1.0f });
		const Matrix viewMatrix = m_Camera.viewMatrix;
		const Matrix modelViewProjection = flipYWorld * viewMatrix * m_Camera.projectionMatrix;

		// Function transforms object space (relative space) directly to screen space and NDC. This is done for every vertex and buffered into the transformedVertices vector
		VertexTransformationFunction(flipYWorld, modelViewProjection, container.mesh->GetVertices(), transformedVertices);

		const std::vector<unsigned int> indices = container.mesh->GetIndices();

		for (int index = 0; index < container.mesh->GetIndices().size() - (container.topology == soft::PrimitiveTopology::TriangleStrip ? 2 : 0); index += (container.topology == soft::PrimitiveTopology::TriangleList ? 3 : 1)) {
			unsigned int i0 = indices[index + 0];
			unsigned int i1 = indices[index + 1];
			unsigned int i2 = indices[index + 2];

			if (i0 == i1 || i2 == i0 || i1 == i2) {
				return;
			}

			triangle.v0 = transformedVertices[i0];
			triangle.v1 = transformedVertices[i1];
			triangle.v2 = transformedVertices[i2];

			if (triangle.v0.position.w <= 0 && triangle.v1.position.w <= 0 && triangle.v2.position.w <= 0)
				continue;

			DrawTriangle(triangle, container);
		}
	}

	//RENDER LOGIC

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, nullptr, m_pFrontBuffer, nullptr);
	SDL_UpdateWindowSurface(m_pWindow);
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

void dae::Renderer::InitializeSamplerStates()
{
	{
		D3D11_SAMPLER_DESC desc{};

		desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

		HRESULT result = m_pDevice->CreateSamplerState(&desc, &m_pSamplerLinear);

		if (FAILED(result)) {
			std::cerr << "Failed to initialize linear sampler state.\n";
			return;
		}

		desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;

		result = m_pDevice->CreateSamplerState(&desc, &m_pSamplerPoint);

		if (FAILED(result)) {
			std::cerr << "Failed to initialize point sampler state.\n";
			return;
		}

		desc.Filter = D3D11_FILTER_ANISOTROPIC;

		result = m_pDevice->CreateSamplerState(&desc, &m_pSamplerAntisotropic);

		if (FAILED(result)) {
			std::cerr << "Failed to initialize antisotropic sampler state.\n";
			return;
		}
	}

	{
		D3D11_RASTERIZER_DESC desc{};

		desc.CullMode = D3D11_CULL_FRONT;
		desc.FrontCounterClockwise = false;
		desc.FillMode = D3D11_FILL_SOLID;
		desc.DepthBias = 0;
		desc.DepthBiasClamp = 0.0f;
		desc.SlopeScaledDepthBias = 0.0f;
		desc.DepthClipEnable = true;
		desc.ScissorEnable = false;
		desc.MultisampleEnable = false;
		desc.AntialiasedLineEnable = false;

		HRESULT result = m_pDevice->CreateRasterizerState(&desc, &m_pCullFront);

		if (FAILED(result)) {
			std::cerr << "Failed to initialize front face culling rasterizer state.\n";
			return;
		}

		desc.CullMode = D3D11_CULL_BACK;
		desc.FrontCounterClockwise = false;

		result = m_pDevice->CreateRasterizerState(&desc, &m_pCullBack);

		if (FAILED(result)) {
			std::cerr << "Failed to initialize back face culling rasterizer state.\n";
			return;
		}

		desc.CullMode = D3D11_CULL_NONE;
		desc.FrontCounterClockwise = false;

		result = m_pDevice->CreateRasterizerState(&desc, &m_pCullNone);

		if (FAILED(result)) {
			std::cerr << "Failed to initialize none face culling rasterizer state.\n";
			return;
		}
	}
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

dae::Effect::Effect(ID3D11Device* pDevice, const std::wstring assetFile) : m_pEffect{ nullptr }, m_pInputLayout{ nullptr }, m_pTechnique{ nullptr }, m_pWorldViewProjection{ nullptr }
{
	m_pEffect = LoadEffect(pDevice, assetFile); // Logs errors
	if (m_pEffect != nullptr) { // Avoids accessing nullptr
		m_pTechnique = m_pEffect->GetTechniqueByName("DefaultTechnique");

		// Vertex Layout
		static constexpr uint32_t numElements{ 5 };
		D3D11_INPUT_ELEMENT_DESC vertexDesc[numElements]{};

		vertexDesc[0].SemanticName = "POSITION";
		vertexDesc[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		vertexDesc[0].AlignedByteOffset = 0;
		vertexDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

		vertexDesc[1].SemanticName = "COLOR";
		vertexDesc[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		vertexDesc[1].AlignedByteOffset = 12;
		vertexDesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

		vertexDesc[2].SemanticName = "TEXCOORD";
		vertexDesc[2].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		vertexDesc[2].AlignedByteOffset = 24;
		vertexDesc[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

		vertexDesc[3].SemanticName = "NORMAL";
		vertexDesc[3].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		vertexDesc[3].AlignedByteOffset = 32;
		vertexDesc[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

		vertexDesc[4].SemanticName = "TANGENT";
		vertexDesc[4].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		vertexDesc[4].AlignedByteOffset = 44;
		vertexDesc[4].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

		// Input Layout
		D3DX11_PASS_DESC passDesc{};
		m_pTechnique->GetPassByIndex(0)->GetDesc(&passDesc);

		const HRESULT result = pDevice->CreateInputLayout(vertexDesc, numElements, passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &m_pInputLayout);
		if (FAILED(result))
			return;

		m_pWorldViewProjection = m_pEffect->GetVariableByName("gWorldViewProj")->AsMatrix();

		if (!m_pWorldViewProjection->IsValid()) {
			std::wcout << "Missing gWorldViewProj from shader.\n";
		}

		m_pWorld = m_pEffect->GetVariableByName("gWorld")->AsMatrix();

		if (!m_pWorld->IsValid()) {
			std::wcout << "Missing gWorld from shader.\n";
		}

		m_pDiffuseMapVariable = m_pEffect->GetVariableByName("gDiffuseMap")->AsShaderResource();

		if (!m_pDiffuseMapVariable->IsValid()) {
			std::cerr << "Missing gDiffuseMap from shader.\n";
		}

		m_pSpecularMapVariable = m_pEffect->GetVariableByName("gSpecularMap")->AsShaderResource();

		if (!m_pSpecularMapVariable->IsValid()) {
			std::cerr << "Missing gSpecularMap from shader.\n";
		}

		m_pNormalMapVariable = m_pEffect->GetVariableByName("gNormalMap")->AsShaderResource();

		if (!m_pNormalMapVariable->IsValid()) {
			std::cerr << "Missing gNormalMap from shader.\n";
		}

		m_pGlossMapVariable = m_pEffect->GetVariableByName("gGlossMap")->AsShaderResource();

		if (!m_pGlossMapVariable->IsValid()) {
			std::cerr << "Missing gGlossMap from shader.\n";
		}

		m_pLightDirection = m_pEffect->GetVariableByName("gLightDirection")->AsVector();

		if (!m_pLightDirection->IsValid()) {
			std::cerr << "Missing gLightDirection from shader.\n";
		}

		m_pCameraOrigin = m_pEffect->GetVariableByName("gCameraOrigin")->AsVector();

		if (!m_pLightDirection->IsValid()) {
			std::cerr << "Missing gCameraOrigin from shader.\n";
		}

		m_pSamplerState = m_pEffect->GetVariableByName("gSamplerState")->AsSampler();

		if (!m_pSamplerState->IsValid()) {
			std::cerr << "Missing gSamplerState from shader.\n";
		}

		m_pRasterizerState = m_pEffect->GetVariableByName("gRasterizerState")->AsRasterizer();

		if (!m_pRasterizerState->IsValid()) {
			std::cerr << "Missing gRasterizerState from shader.\n";
		}
	}
}

dae::Effect::~Effect()
{
	_RELEASE_DX11_PTR(m_pInputLayout)
	_RELEASE_DX11_PTR(m_pTechnique)
	_RELEASE_DX11_PTR(m_pEffect)
}

void dae::Effect::SetWorldViewProj(const Matrix& m)
{
	if (m_pWorldViewProjection == nullptr)
		return;
	if (!m_pWorldViewProjection->IsValid())
		return;

	m_pWorldViewProjection->SetMatrix((float*)&m);
}

void dae::Effect::SetWorld(const Matrix& m)
{
	if (m_pWorld == nullptr)
		return;
	if (!m_pWorld->IsValid())
		return;

	m_pWorld->SetMatrix((float*)&m);
}

void dae::Effect::SetDiffuseMap(const Texture* diffuse)
{
	if (!diffuse)
		return;
	if (m_pDiffuseMapVariable)
		m_pDiffuseMapVariable->SetResource(diffuse->GetSRV());
}

void dae::Effect::SetSpecularMap(const Texture* diffuse)
{
	if (!diffuse)
		return;
	if (m_pSpecularMapVariable)
		m_pSpecularMapVariable->SetResource(diffuse->GetSRV());
}

void dae::Effect::SetNormalMap(const Texture* diffuse)
{
	if (!diffuse)
		return;
	if (m_pNormalMapVariable)
		m_pNormalMapVariable->SetResource(diffuse->GetSRV());
}

void dae::Effect::SetGlossMap(const Texture* diffuse)
{
	if (!diffuse)
		return;
	if (m_pGlossMapVariable)
		m_pGlossMapVariable->SetResource(diffuse->GetSRV());
}

void dae::Effect::SetCameraOrigin(const Vector3& v)
{
	if (m_pCameraOrigin)
		m_pCameraOrigin->SetFloatVector((float*) & v);
}

void dae::Effect::SetLightDirection(const Vector3& v)
{
	if (m_pLightDirection)
		m_pLightDirection->SetFloatVector((float*)&v);
}

void dae::Effect::SetSamplerState(ID3D11SamplerState* v)
{
	if (m_pSamplerState && m_pSamplerState->IsValid()) {
		m_pSamplerState->SetSampler(0, v);
	}
}

void dae::Effect::SetRasterizerState(ID3D11RasterizerState* v)
{
	if (m_pRasterizerState && m_pRasterizerState->IsValid())
		m_pRasterizerState->SetRasterizerState(0, v);
}

#pragma endregion Effect

#pragma region Mesh
dae::Mesh::Mesh(ID3D11Device* pDevice, const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) : m_NumIndices{ 0 }, m_pIndexBuffer{ nullptr }, m_pVertexBuffer{ nullptr }
{
	m_Vertices = vertices;
	m_Indices = indices;
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
	_RELEASE_DX11_PTR(m_pVertexBuffer)
	_RELEASE_DX11_PTR(m_pIndexBuffer)
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

void dae::Scene::AddTexture(Texture* texture)
{
	m_Textures.emplace_back(texture);
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

	for (Texture* texture : m_Textures) {
		delete texture;
	}
	m_Effects.clear();
}

void dae::Scene::InitializeScene(ID3D11Device* pDevice)
{
	Mesh* helloTriangle;
	Mesh* vehicle;
	Mesh* fireFX;

	{
		std::vector<dae::Vertex> vertices{
			{ { 0.f, 0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f } },
			{ { 0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } },
			{ { -0.5f, -0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f } }
		};
		std::vector<uint32_t> indices{ 0, 1, 2 };
		helloTriangle = new dae::Mesh(pDevice, vertices, indices);
	}

	{
		std::vector<dae::Vertex> vertices{};
		std::vector<uint32_t> indices{};

		if (Utils::ParseOBJ("./resources/vehicle.obj", vertices, indices)) {
			std::cout << "Parsed vehicle obj" << std::endl;
		}
		vehicle = new Mesh(pDevice, vertices, indices);
	}

	{
		std::vector<dae::Vertex> vertices{};
		std::vector<uint32_t> indices{};

		if (Utils::ParseOBJ("./resources/fireFX.obj", vertices, indices)) {
			std::cout << "Parsed firefx obj" << std::endl;
		}
		fireFX = new Mesh(pDevice, vertices, indices);
	}

	AddMesh(helloTriangle);
	AddMesh(vehicle);
	AddMesh(fireFX);

	Texture* vehicle_Diffuse = new Texture(pDevice, "./resources/vehicle_diffuse.png");
	Texture* vehicle_Specular = new Texture(pDevice, "./resources/vehicle_specular.png");
	Texture* vehicle_Gloss = new Texture(pDevice, "./resources/vehicle_gloss.png");
	Texture* vehicle_Normal = new Texture(pDevice, "./resources/vehicle_normal.png");

	Texture* fireFx_Diffuse = new Texture(pDevice, "./resources/fireFX_diffuse.png");

	AddTexture(vehicle_Diffuse);
	AddTexture(vehicle_Specular);
	AddTexture(vehicle_Gloss);
	AddTexture(vehicle_Normal);

	AddTexture(fireFx_Diffuse);

	Effect* posColEffect = new dae::Effect(pDevice, L"./resources/PosCol3D.fx");
	Effect* fireFX_Effect = new dae::Effect(pDevice, L"./resources/fireFX_Effect.fx");

	AddEffect(posColEffect);
	AddEffect(fireFX_Effect);

	{
		Container container{};

		container.effect = posColEffect;
		container.mesh = vehicle;
		container.diffuseMap = vehicle_Diffuse;
		container.normalMap = vehicle_Normal;
		container.glossMap = vehicle_Gloss;
		container.specularMap = vehicle_Specular;

		AddContainer(container);
	}

	{
		Container container{};

		container.effect = fireFX_Effect;
		container.mesh = fireFX;
		container.diffuseMap = fireFx_Diffuse;
		container.normalMap = vehicle_Normal;
		container.glossMap = vehicle_Gloss;
		container.specularMap = vehicle_Specular;
		container.isFlame = true;

		AddContainer(container);
	}
}

void dae::Scene::GenerateMips(ID3D11DeviceContext* pDeviceContext)
{
	for (Texture* texture : m_Textures) {
		pDeviceContext->GenerateMips(texture->GetSRV());
	}
}

const std::vector<Container>& dae::Scene::GetContainers() const
{
	return m_Containers;
}

std::vector<Container>& dae::Scene::GetContainersM()
{
	return m_Containers;
}

#pragma endregion Scene

#pragma region Software Rasterizer Functions

void Renderer::VertexTransformationFunction(const Matrix& objectToWorld, const Matrix& modelViewProjection, const std::vector<Vertex>& vertices_in, std::vector<soft::Vertex_Out>& vertices_out) const {
	// Resizes, but doesn't change capacity so the vector only scales up the capacity if necessary
	if (vertices_out.size() != vertices_in.size())
		vertices_out.resize(vertices_in.size());

	for (int index = 0; index < vertices_in.size(); ++index) {
		VertexInformation(objectToWorld, modelViewProjection, vertices_in[index], vertices_out[index]);
	}
}

void Renderer::VertexInformation(const Matrix& objectToWorld, const Matrix& modelViewProjection, const Vertex& vin, soft::Vertex_Out& vout) const {
	vout = vin;

	const Vector4 position = modelViewProjection.TransformPoint(Vector4(vin.Position, 1.0f));

	// Use only rotation and scale.
	vout.normal = objectToWorld.TransformVector(vin.Normal);
	vout.tangent = objectToWorld.TransformVector(vin.Tangent);

	vout.normal.y *= -1.0f;
	vout.tangent.y *= -1.0f;

	vout.worldPosition = objectToWorld.TransformPoint(position);

	vout.viewDirection = (vout.worldPosition - m_Camera.origin).Normalized();

	if (position.w == 0.0f) { // Avoid dividing by 0 just in case
		vout.xyNorm = { 0.0f, 0.0f };
	}
	else {
		const float z = position.z / position.w;

		if (z != 0.0f)
			vout.xyNorm = { position.x / position.w, position.y / position.w };
		else
			vout.xyNorm = { 0.0f, 0.0f };
	}
	vout.position = position;
	vout.positionScaled = Vector3(position) / position.w;
}

void Renderer::DrawTriangle(const soft::Triangle& triangle, const Container& material) {
	MinMaxAABB(triangle, m_Max, m_Min);

	Int2 pixelCoordMax{ vToi(Ceil(VectorRangeToPixelCoord(m_Max))) };
	Int2 pixelCoordMin{ vToi(Floor(VectorRangeToPixelCoord(m_Min))) };

	pixelCoordMax.x += 1;
	pixelCoordMax.y += 1;

	pixelCoordMin.x -= 1;
	pixelCoordMin.y -= 1;

	Int_AABB triangleBounds{ pixelCoordMin, {pixelCoordMax.x - pixelCoordMin.x, -(pixelCoordMax.y - pixelCoordMin.y) + m_Height} };

	if (!isIntersect(triangleBounds, m_ScreenBounds)) { // Instead of culling by triangle points instead cull by 2d bounding box
		return;
	}

	const Vector2 v0 = triangle.v0.xyNorm;
	const Vector2 v1 = triangle.v1.xyNorm;
	const Vector2 v2 = triangle.v2.xyNorm;

	const Vector2 c0 = VectorRangeToPixelCoord(v0);
	const Vector2 c1 = VectorRangeToPixelCoord(v1);
	const Vector2 c2 = VectorRangeToPixelCoord(v2);

	const Vector2 r_min = v_Clamp(v_min(c0, c1, c2), { 0, 0 }, { static_cast<float>(m_Width), static_cast<float>(m_Height) });
	const Vector2 r_max = v_Clamp(v_max(c0, c1, c2), { 0, 0 }, { static_cast<float>(m_Width), static_cast<float>(m_Height) });

	const Vector3 invZ = { 1.0f / triangle.v0.position.w, 1.0f / triangle.v1.position.w, 1.0f / triangle.v2.position.w };

	soft::VS_OUT varryings{ material };

	for (int x = int(r_min.x); x < int(r_max.x); ++x) {
		for (int y = int(r_min.y); y < int(r_max.y); ++y) {
			const int pixelIndex = x + y * m_Width;
			ColorRGB finalColor{ colors::White };

			const Vector2 pixel{ center(x, y) };

			bool isBackFace{ 0 };

			if (!isPixelInTriangle(c0, c1, c2, pixel, material.isFlame, isBackFace))
				continue;

			Vector3 barCoord = isBackFace ? GetBarycentricCoord(c0, c1, c2, pixel) : GetBarycentricCoord(c0, c1, c2, pixel);

			const float invInterpolatedW = 1.0f / ((invZ.x) * barCoord.x + (invZ.y) * barCoord.y + (invZ.z) * barCoord.z);

			const float interpolatedZ = (triangle.v0.positionScaled.z * invZ.x * barCoord.x + triangle.v1.positionScaled.z * invZ.y * barCoord.y + triangle.v2.positionScaled.z * invZ.z * barCoord.z) * invInterpolatedW;

			if (m_pScene->renderSettings.useDepth) {
				if (m_pDepthBufferPixels[pixelIndex] < interpolatedZ) {
					continue;
				}
			}


			// Interpolated values
			Vector2 texCoord{ (triangle.v0.uv * invZ.x * barCoord.x + triangle.v1.uv * invZ.y * barCoord.y + triangle.v2.uv * invZ.z * barCoord.z) * invInterpolatedW };
			Vector3 normal{ (triangle.v0.normal * invZ.x * barCoord.x + triangle.v1.normal * invZ.y * barCoord.y + triangle.v2.normal * invZ.z * barCoord.z) * invInterpolatedW };
			Vector3 tangent{ (triangle.v0.tangent * invZ.x * barCoord.x + triangle.v1.tangent * invZ.y * barCoord.y + triangle.v2.tangent * invZ.z * barCoord.z) * invInterpolatedW };
			Vector3 viewDirection{ (triangle.v0.viewDirection * invZ.x * barCoord.x + triangle.v1.viewDirection * invZ.y * barCoord.y + triangle.v2.viewDirection * invZ.z * barCoord.z) * invInterpolatedW };
			// End

			varryings.vTexCoord = texCoord;
			varryings.vNormal = normal;
			varryings.vTangent = tangent;
			varryings.vViewDirection = viewDirection;

			static const int blueMask = 0xFF0000, greenMask = 0xFF00, redMask = 0xFF;

			uint32_t value = m_pBackBufferPixels[pixelIndex];

			ColorRGB existingPixel{ float((value & redMask)) / 255.f, float((value & greenMask) >> 8) / 255.f, float((value & blueMask) >> 16) / 255.f };

			float alpha{ 1.0f };
			bool pass = PixelShading(finalColor, varryings, alpha, material.isFlame);

			if (!pass && !m_pScene->renderSettings.visualizeDepth)
				continue;

			finalColor.MaxToOne();
			finalColor = ColorRGB::Lerp(existingPixel, finalColor, 1.0f - powf(1.0f - alpha, 2.0f)); // finalColor + existingPixel * (1.0f - alpha); //


			// Bumping up the luminosity using the sine curve to lift the lower values
			// finalColor.r = sinf(finalColor.r * M_PI / 2.0f);
			// finalColor.g = sinf(finalColor.g * M_PI / 2.0f);
			// finalColor.b = sinf(finalColor.b * M_PI / 2.0f);

			if (alpha == 1.0f)
				m_pDepthBufferPixels[pixelIndex] = interpolatedZ;

			if (m_pScene->renderSettings.visualizeDepth) {
				finalColor = { interpolatedZ, interpolatedZ, interpolatedZ };
			}

			m_pBackBufferPixels[pixelIndex] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}
}

bool Renderer::PixelShading(ColorRGB& fragColor, const soft::VS_OUT& in, float& alpha, bool isFlame) const {
	const soft::RenderSettings& rs{ m_pScene->renderSettings };

	alpha = 1.0f;
	if (in.container.diffuseMap != nullptr) {
		alpha = in.container.diffuseMap->SampleRGBA(in.vTexCoord.x, in.vTexCoord.y).a;
		if (alpha < 0.05f)
			return false;
	}

	ColorRGB pixelColor{ colors::White };
	const Matrix tangentSpaceAxis = GetTangentSpaceAxis(in.vTangent, in.vNormal);

	Vector3 transformedNormal = m_pScene->renderSettings.useNormalMap ? tangentSpaceAxis.TransformVector(in.container.normalMap->SampleNormal(in.vTexCoord)) : in.vNormal;

	const float observable_area = GetObservableArea(transformedNormal);

	if (in.container.diffuseMap != nullptr && rs.shadingMode == soft::RenderSettings::Combined || rs.shadingMode == soft::RenderSettings::Diffuse)
		pixelColor = GetLambertColor(in.container.diffuseMap->Sample(in.vTexCoord), 7.0f);

	if ((rs.shadingMode == soft::RenderSettings::Combined || rs.shadingMode == soft::RenderSettings::Specular) && !isFlame) {
		const float hcSpecularMultiplier = 1.0f;
		const float hcGlossMultiplier = 25.0f;

		const float specularFactor{ in.container.specularMap->Sample(in.vTexCoord).r }; // Jusk keeping r because specular is black and white
		const float glossFactor{ in.container.glossMap->Sample(in.vTexCoord).r }; // And same here

		const float phongValue{ Phong(-m_LightDirection, transformedNormal, in.vViewDirection, specularFactor * hcSpecularMultiplier, glossFactor * hcGlossMultiplier) };

		pixelColor += ColorRGB{ phongValue, phongValue, phongValue };
	}

	if ((rs.shadingMode == soft::RenderSettings::Combined || rs.shadingMode == soft::RenderSettings::ObservedArea) && !isFlame)
		pixelColor *= observable_area;

	//pixelColor.MaxToOne();
	fragColor = pixelColor;
	return true;
}

bool Renderer::isPixelInTriangle(const Vector2& c0, const Vector2& c1, const Vector2& c2, const Vector2& pixel, bool isFlame, bool& isBackFace) {
	bool z0 = Vector2::Cross(pixel - c0, c1 - c0) < 0;
	bool z1 = Vector2::Cross(pixel - c1, c2 - c1) < 0;
	bool z2 = Vector2::Cross(pixel - c2, c0 - c2) < 0;

	isBackFace = (!z0 && !z1 && !z2);
	bool isFrontFace = (z0 && z1 && z2);

	bool none = isBackFace || isFrontFace;

	if (isFlame)
		return isFrontFace;

	switch (m_pScene->renderSettings.cullMode) {
		case soft::RenderSettings::Back:
			return isFrontFace;
		case soft::RenderSettings::Front:
			return isBackFace;
		case soft::RenderSettings::None:
			return none;
	}
}

Vector2 Renderer::NormlPixelToScreen(const Vector2& normlPixel) const {
	return { normlPixel.x / float(m_Width), normlPixel.y / float(m_Height) };
}

Vector2 Renderer::center(int x, int y) {
	return { float(x) + 0.5f, float(y) + 0.5f };
}

void Renderer::ClearBuffers() const {
	ColorRGB finalColor{ m_pScene->renderSettings.useUniformClearColor ? ColorRGB{ clearColor[0], clearColor[1], clearColor[2] } : clearColorSoftware};
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
			m_pDepthBufferPixels[px + (py * m_Width)] = 1.0f;
		}
	}
}

Vector3 Renderer::GetBarycentricCoord(const Vector2& c0, const Vector2& c1, const Vector2& c2, const Vector2& pixel) {
	const float totalArea = fabsf(Vector2::Cross(c1 - c0, c2 - c0));

	const float invArea = 1.0f / totalArea;

	const float e0 = Vector2::Cross(c1 - c0, pixel - c0);
	const float e1 = Vector2::Cross(c2 - c1, pixel - c1);
	const float e2 = Vector2::Cross(c0 - c2, pixel - c2);

	return Vector3{ e1, e2, e0 } * invArea;
}

Vector2 Renderer::v_max(const Vector2& v1, const Vector2& v2, const Vector2& v3) {
	return { fmaxf(fmaxf(v1.x, v2.x), v3.x), fmaxf(fmaxf(v1.y, v2.y), v3.y) };
}

Vector2 Renderer::v_min(const Vector2& v1, const Vector2& v2, const Vector2& v3) {
	return { fminf(fminf(v1.x, v2.x), v3.x), fminf(fminf(v1.y, v2.y), v3.y) };
}

Vector2 Renderer::v_Clamp(const Vector2& v, const Vector2& min, const Vector2& max) {
	return { Clamp(v.x, min.x, max.x), Clamp(v.y, min.y, max.y) };
}

bool Renderer::isClose(const Vector2& v0, const Vector2& p, float distance) {
	const float dx = fabsf(v0.x - p.x);
	const float dy = fabsf(v0.y - p.y);

	return 0.5f * (dx + dy + fmaxf(dx, dy)) < distance;
}

void Renderer::MinMaxAABB(const soft::Triangle& screenSpace, Vector2& max, Vector2& min) {
	max.x = fmaxf(screenSpace.v0.xyNorm.x, fmaxf(screenSpace.v1.xyNorm.x, screenSpace.v2.xyNorm.x));
	max.y = fmaxf(screenSpace.v0.xyNorm.y, fmaxf(screenSpace.v1.xyNorm.y, screenSpace.v2.xyNorm.y));

	min.x = fminf(screenSpace.v0.xyNorm.x, fminf(screenSpace.v1.xyNorm.x, screenSpace.v2.xyNorm.x));
	min.y = fminf(screenSpace.v0.xyNorm.y, fminf(screenSpace.v1.xyNorm.y, screenSpace.v2.xyNorm.y));
}

Vector2 Renderer::VectorRangeToPixelCoord(const Vector2& v) const {
	Vector2 result = { ((v.x * 0.5f) + 0.5f) * m_Width, ((v.y * 0.5f) + 0.5f) * m_Height };

	return result;
}

Vector2 Renderer::Ceil(const Vector2& v) {
	return { ceilf(v.x), ceilf(v.y) };
}

Vector2 Renderer::Floor(const Vector2& v) {
	return { floorf(v.x), floorf(v.y) };
}

Vector2 Renderer::PixelCoordToScreenCoord(const Vector2& v) const {
	float x = v.x / m_Width * 2.f - 1.f;
	float y = v.y / m_Height * 2.f - 1.f;

	return { x, y };
}

float Renderer::GetObservableArea(const Vector3& normal) const {
	return fmaxf(Vector3::Dot(normal, -m_LightDirection), 0.0f);
}

ColorRGB Renderer::GetLambertColor(const ColorRGB& cd, float kd) {
	return cd * kd / float(M_PI);
}

Matrix Renderer::GetTangentSpaceAxis(const Vector3& tangent, const Vector3& normal) {
	Vector3 binormal = Vector3::Cross(normal, tangent);
	return Matrix{ tangent, binormal, normal, {0, 0, 0} };
}

float Renderer::Phong(const Vector3& l, const Vector3& n, const Vector3& v, float ks, float e) {
	const Vector3 r{ l - (n * 2.0f * Vector3::Dot(l,n)) };
	const float cos_a{ fmaxf(Vector3::Dot(r,v), 0.0f) };

	return ks * powf(cos_a, e);
}

Int2 Renderer::vToi(const Vector2& v) {
	return { int(v.x), int(v.y) };
}

#pragma endregion