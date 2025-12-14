#pragma once

class ID3DX11Effect;
class ID3DX11EffectTechnique;
class ID3D11InputLayout;
class ID3DX11Device;

#include <string>

namespace dae {
	class Effect {
		ID3DX11Effect* m_pEffect;
		ID3DX11EffectTechnique* m_pTechnique;
		ID3D11InputLayout* m_pInputLayout;

		static ID3DX11Effect* LoadEffect(ID3D11Device* pDevice, const std::wstring assetFile);

	public:
		explicit Effect(ID3D11Device* pDevice, const std::wstring assetFile);
		~Effect();

		ID3D11InputLayout* GetInputLayout() const { return m_pInputLayout; }
		ID3DX11EffectTechnique* GetTechnique() const { return m_pTechnique; }
	};
}