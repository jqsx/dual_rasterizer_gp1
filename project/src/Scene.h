#ifndef DAE_SCENE_DX11
#define DAE_SCENE_DX11

#include <vector>

class ID3D11Device;

namespace dae {
	class Mesh;
	class Effect;

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

#endif