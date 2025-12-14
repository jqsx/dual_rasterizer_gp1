#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>

#include "Mesh.h"

using namespace dae;

dae::Mesh::Mesh(ID3D11Device* pDevice, std::vector<Vertex> vertices, std::vector<unsigned int> indices)
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
		return;

	m_NumIndices = static_cast<uint32_t>(indices.size());
	bd.ByteWidth = sizeof(uint32_t) * m_NumIndices;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;

	initData.pSysMem = indices.data();
	result = pDevice->CreateBuffer(&bd, &initData, &m_pIndexBuffer);
	if (FAILED(result))
		return;
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
