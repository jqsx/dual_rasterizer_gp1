//External includes
#include "SDL.h"
#include "SDL_surface.h"

// Standard includes
#include <iostream>

#include "Utils.h"

#include "Texture.h"

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

void Renderer::Update(const Timer* pTimer, bool leftClick, bool rightClick)
{
	m_Camera.aspect = float(m_Width) / float(m_Height);
	m_Camera.Update(pTimer, leftClick, rightClick);
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

		Matrix projView = m_Camera.viewMatrix * m_Camera.projectionMatrix;

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
		container.effect->SetSamplerState(m_pSamplerAntisotropic);

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

void dae::Renderer::InitializeSamplerStates()
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

		AddContainer(container);
	}
}

void dae::Scene::GenerateMips(ID3D11DeviceContext* pDeviceContext)
{
	for (Texture* texture : m_Textures) {
		pDeviceContext->GenerateMips(texture->GetSRV());
	}
}

#pragma endregion Scene