struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Color : COLOR;
    float2 TexCoord : TEXCOORD;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float3 WorldPosition : POSITION;
    float3 ViewDirection : VIEWDIRECTION;
    float3 Color : COLOR;
    float2 TexCoord : TEXCOORD;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
};

RasterizerState gRasterizerState : CullMode;

RasterizerState _gRasterizerState
{
    CullMode = back;
    FrontCounterClockwise = false;
};

DepthStencilState gDepthStencilState
{
    DepthEnable = true;
    DepthWriteMask = zero;
    DepthFunc = less;
    StencilEnable = false;
};

BlendState gBlendState
{
    BlendEnable[0] = true;
    SrcBlend = src_alpha;
    DestBlend = inv_src_alpha;
    BlendOp = add;
    SrcBlendAlpha = zero;
    DestBlendAlpha = zero;
    BlendOpAlpha = add;
    RenderTargetWriteMask[0] = 0x0F;
};

float4x4 gWorldViewProj : WorldViewProjection;
float4x4 gWorld : WorldMatrix;

float3 gLightDirection : LightDirection;
float3 gCameraOrigin : CameraOrigin;

Texture2D gDiffuseMap : DiffuseMap;
Texture2D gGlossMap : GlossMap;
Texture2D gNormalMap : NormalMap;
Texture2D gSpecularMap : SpecularMap;

SamplerState gSamplerState : TextureSampleState;

float3 Lambert(float3 diffuse, float ks)
{
    return diffuse * ks;
}

float3 MaxToOne(float3 color)
{
    float maxValue = max(color.x, max(color.y, color.z));
    if (maxValue > 1.f)
        return color /= maxValue;
    return color;
}

VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT) 0;

    output.Position = mul(float4(input.Position, 1.0f), gWorldViewProj);
    output.TexCoord = input.TexCoord;

    return output;
}

float4 PS(VS_OUTPUT input) : SV_TARGET
{
    float4 diffuse = gDiffuseMap.Sample(gSamplerState, input.TexCoord);
    
    return float4(MaxToOne(Lambert(diffuse.rgb, 7.0f)), diffuse.a);
}

technique11 DefaultTechnique
{
    pass P0
    {
        SetRasterizerState(_gRasterizerState);
        SetDepthStencilState(gDepthStencilState, 0);
        SetBlendState(gBlendState, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }
}