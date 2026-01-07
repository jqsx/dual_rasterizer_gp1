//External includes
#include "SDL.h"
#include "SDL_surface.h"
#include "SDL_image.h"
#include "SDL_syswm.h"
#undef main

//Standard includes
#include <iostream>

//Project includes
#include "Timer.h"
#include "Renderer.h"
#if defined(_DEBUG)
	#include "LeakDetector.h"
#endif

using namespace dae;

void ShutDown(SDL_Window* pWindow)
{
	SDL_DestroyWindow(pWindow);
	SDL_Quit();
}

int main(int argc, char* args[])
{
	//Unreferenced parameters
	(void)argc;
	(void)args;

	// Leak detection
	#if defined(_DEBUG)
		LeakDetector detector{};
		//detector.BreakOnAllocationId(288);
	#endif

	//Create window + surfaces
	SDL_Init(SDL_INIT_VIDEO);

	const uint32_t width = 640;
	const uint32_t height = 480;

	SDL_Window* pWindow = SDL_CreateWindow(
		"DirectX - Franciszek Rakowiecki GD10",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		width, height, 0);

	if (!pWindow)
		return 1;

	//Initialize "framework"
	const auto pTimer = new Timer();
	const auto pRenderer = new Renderer(pWindow);

	//Start loop
	pTimer->Start();
	float printTimer = 0.f;
	bool isLooping = true;

	bool lmb{0}, rmb{ 0 };


	bool useSoftwareRasterizer = false;

	bool printFPS{ 0 };

	Scene* scene = pRenderer->GetScene();
	
	while (isLooping)
	{
		//--------- Get input events ---------
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
			case SDL_QUIT:
				isLooping = false;
				break;
			case SDL_KEYUP:
				//Test for a key
				if (e.key.keysym.scancode == SDL_SCANCODE_F1) {
					useSoftwareRasterizer = !useSoftwareRasterizer;
					std::cout << (useSoftwareRasterizer ? "Using software rasterizer" : "Using hardware rasterizer") << std::endl;
				}
				else if (e.key.keysym.scancode == SDL_SCANCODE_F2) {
					scene->renderSettings.hasRotation = !scene->renderSettings.hasRotation;
					std::cout << "Rotateion: " << scene->renderSettings.hasRotation << std::endl;
				}
				else if (e.key.keysym.scancode == SDL_SCANCODE_F3) {
					scene->renderSettings.drawFireFx = !scene->renderSettings.drawFireFx;
					std::cout << "FireFX: " << scene->renderSettings.drawFireFx << std::endl;
				}
				else if (e.key.keysym.scancode == SDL_SCANCODE_F4) {
					scene->renderSettings.samplingState = soft::RenderSettings::SamplingState((int(scene->renderSettings.samplingState) + 1) % 3);

					std::cout << "Current sampling state: ";
					switch (scene->renderSettings.samplingState) {
					case soft::RenderSettings::Point:
						std::cout << "Point\n";
						break;
					case soft::RenderSettings::Linear:
						std::cout << "Linear\n";
						break;
					case soft::RenderSettings::Anisotropic:
						std::cout << "Anisotropic\n";
						break;
					}
				}
				else if (e.key.keysym.scancode == SDL_SCANCODE_F5) {
					scene->renderSettings.shadingMode = soft::RenderSettings::ShadingMode((int(scene->renderSettings.shadingMode) + 1) % 4);

					std::cout << "Current shading mode: ";
					switch (scene->renderSettings.shadingMode) {
					case soft::RenderSettings::Combined:
						std::cout << "Combined\n";
						break;
					case soft::RenderSettings::Diffuse:
						std::cout << "Diffuse\n";
						break;
					case soft::RenderSettings::ObservedArea:
						std::cout << "ObservableArea\n";
						break;
					case soft::RenderSettings::Specular:
						std::cout << "Specular\n";
						break;
					}
				}
				else if (e.key.keysym.scancode == SDL_SCANCODE_F6) {
					scene->renderSettings.useNormalMap = !scene->renderSettings.useNormalMap;
					std::cout << "Use Normal Map: " << scene->renderSettings.useNormalMap << std::endl;
				}
				else if (e.key.keysym.scancode == SDL_SCANCODE_F7) {
					scene->renderSettings.visualizeDepth = !scene->renderSettings.visualizeDepth;
					std::cout << "Depth visualizer: " << scene->renderSettings.visualizeDepth << std::endl;
				}
				else if (e.key.keysym.scancode == SDL_SCANCODE_F8) {
					scene->renderSettings.showTriangleBounds = !scene->renderSettings.showTriangleBounds;
					std::cout << "Triangle bounds visualizer: " << scene->renderSettings.showTriangleBounds << std::endl;
				}
				else if (e.key.keysym.scancode == SDL_SCANCODE_F9) {
					scene->renderSettings.cullMode = soft::RenderSettings::CullMode((int(scene->renderSettings.cullMode) + 1) % 3);

					std::cout << "Current cull mode: ";
					switch (scene->renderSettings.cullMode) {
					case soft::RenderSettings::Front:
						std::cout << "Front\n";
						break;
					case soft::RenderSettings::Back:
						std::cout << "Back\n";
						break;
					case soft::RenderSettings::None:
						std::cout << "None\n";
						break;
					}
				}
				else if (e.key.keysym.scancode == SDL_SCANCODE_F10) {
					scene->renderSettings.useUniformClearColor = !scene->renderSettings.useUniformClearColor;
					std::cout << "Uniform clear color: " << scene->renderSettings.useUniformClearColor << std::endl;
				}
				else if (e.key.keysym.scancode == SDL_SCANCODE_F11) {
					printFPS = !printFPS;
					std::cout << "Printing fps: " << printFPS << std::endl;
				}
				break;
			case SDL_MOUSEBUTTONUP:
				if (e.button.button == SDL_BUTTON_LEFT)
					lmb = false;
				else if (e.button.button == SDL_BUTTON_RIGHT) {
					rmb = false;
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				if (e.button.button == SDL_BUTTON_LEFT)
					lmb = true;
				else if (e.button.button == SDL_BUTTON_RIGHT) {
					rmb = true;
				}
				break;
			default:
				break;
			}
		}

		SDL_SetRelativeMouseMode(lmb || rmb ? SDL_TRUE : SDL_FALSE);

		//--------- Update ---------
		pRenderer->Update(pTimer, lmb, rmb, useSoftwareRasterizer);

		//--------- Render ---------
		if (useSoftwareRasterizer)
			pRenderer->RenderSoftwareRasterizer();
		else
			pRenderer->Render();

		//--------- Timer ---------
		pTimer->Update();
		printTimer += pTimer->GetElapsed();
		if (printTimer >= 1.f)
		{
			printTimer = 0.f;
			if (printFPS)
				std::cout << "dFPS: " << pTimer->GetdFPS() << std::endl;
		}
	}
	pTimer->Stop();

	//Shutdown "framework"
	delete pRenderer;
	delete pTimer;

	ShutDown(pWindow);

	return 0;
}