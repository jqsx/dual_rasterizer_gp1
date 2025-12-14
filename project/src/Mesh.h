#ifndef DAE_MESH_DX11
#define DAE_MESH_DX11

#include "ColorRGB.h"
#include "Vector3.h"
#include <vector>

class ID3D11Buffer;
class ID3D11Device;

namespace dae {
	struct Vertex {
		Vector3 Position;
		ColorRGB Color;
	};

	class Mesh {
		ID3D11Buffer* m_pVertexBuffer;
		ID3D11Buffer* m_pIndexBuffer;

		uint32_t m_NumIndices;
	public:
		Mesh(ID3D11Device* pDevice, std::vector<Vertex> vertices, std::vector<unsigned int> indices);
		~Mesh();

		ID3D11Buffer* GetVertexBuffer() const { return m_pVertexBuffer; }
		ID3D11Buffer* GetIndexBuffer() const { return m_pIndexBuffer; }
	};
}

#endif