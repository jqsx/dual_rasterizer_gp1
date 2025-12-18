//External includes
#include "SDL.h"
#include "SDL_surface.h"

// Standard includes
#include <iostream>

#include "Scene.h"

//Project includes
#include "Renderer.h"

using namespace dae;

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
	if (!m_IsInitialized)
		return;


}


void Renderer::Render() const
{
	if (!m_IsInitialized)
		return;

	constexpr float color[4] = { 0.0f, 0.0f, 0.3f, 1.0f };

	m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, color);
	m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Invoke draw calls

	// present backbuffer

	for (const Container& container : m_pScene->GetContainers()) {
		RenderUsing(container.mesh, container.effect);
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

void dae::Renderer::RenderUsing(const Mesh* pMesh, const Effect* pEffect) const
{

}
