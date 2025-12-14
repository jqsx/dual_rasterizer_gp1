#include <iostream>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <d3d11.h>
#include <d3dx11effect.h>

#include "Effect.h"

using namespace dae;

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
				std::wcout << pErrors[i];
			}

			pErrorBlob->Release();
			pErrorBlob = nullptr;
		}
		else {
			std::wcout << "EffectLoader: Failed to CreateEffectFromFile!\nPath: " << assetFile << std::endl;
			return nullptr;
		}
	}

	return pEffect;
}

dae::Effect::Effect(ID3D11Device* pDevice, const std::wstring assetFile)
{
	m_pEffect = LoadEffect(pDevice, assetFile); // Logs errors
	if (m_pEffect != nullptr) { // Avoids accessing nullptr
		m_pTechnique = m_pEffect->GetTechniqueByName("DefaultTechnique");

		// Vertex Layout
		static constexpr uint32_t numElements{ 2 };
		D3D11_INPUT_ELEMENT_DESC vertexDesc[numElements]{};

		vertexDesc[0].SemanticName = "POSITION";
		vertexDesc[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		vertexDesc[0].AlignedByteOffset = 0;
		vertexDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

		vertexDesc[1].SemanticName = "COLOR";
		vertexDesc[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		vertexDesc[1].AlignedByteOffset = 12;
		vertexDesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

		// Input Layout
		D3DX11_PASS_DESC passDesc{};
		m_pTechnique->GetPassByIndex(0)->GetDesc(&passDesc);

		const HRESULT result = pDevice->CreateInputLayout(vertexDesc, numElements, passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &m_pInputLayout);
		if (FAILED(result))
			return;
	}
}

dae::Effect::~Effect()
{
	if (m_pInputLayout != nullptr) {
		m_pInputLayout->Release();
		m_pInputLayout = nullptr;
	}
	if (m_pTechnique != nullptr) {
		m_pTechnique->Release();
		m_pTechnique = nullptr;
	}
	if (m_pEffect != nullptr) {
		m_pEffect->Release();
		m_pEffect = nullptr;
	}
}
