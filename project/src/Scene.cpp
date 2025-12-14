#include "Mesh.h"
#include "Effect.h"

#include <string>
#include <vector>

#include "Scene.h"

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
	std::vector<Vertex> vertices{
		{ { 0.f, 0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f } },
		{ { 0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } },
		{ { -0.5f, -0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f } }
	};
	std::vector<uint32_t> indices{ 0, 1, 2 };
	Mesh* helloTriangle = new Mesh(pDevice, vertices, indices);

	AddMesh(helloTriangle);

	Effect* posColEffect = new Effect(pDevice, L"./resources/PosCol3D.fx");

	AddEffect(posColEffect);

	Container container{};

	container.effect = posColEffect;
	container.mesh = helloTriangle;

	AddContainer(container);
}
