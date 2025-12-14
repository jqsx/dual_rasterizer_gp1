#pragma once

#include <vector>

#include "Mesh.h"
#include "Effect.h"

class ID3D11Device;

namespace dae {
	//class Mesh;
	//class Effect;

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
}