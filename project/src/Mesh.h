#pragma once

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
		explicit Mesh(ID3D11Device* pDevice, const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
		~Mesh();

		ID3D11Buffer* GetVertexBuffer() const { return m_pVertexBuffer; }
		ID3D11Buffer* GetIndexBuffer() const { return m_pIndexBuffer; }
	};
}