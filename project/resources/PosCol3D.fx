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

DepthStencilState gDepthStencilState
{
    DepthEnable = true;
    DepthWriteMask = all;
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

float GetObservableArea(float3 worldNormal)
{
    return max(dot(worldNormal, -gLightDirection), 0.0f);
}

float3 SampleNormal(float2 coord)
{
    float4 color = gNormalMap.Sample(gSamplerState, coord);
    return float3(color.r * 2.0f - 1.0f, color.g * 2.0f - 1.0f, (color.b - 0.5f) * 2.0f);
}

float3 MaxToOne(float3 color)
{
    float maxValue = max(color.x, max(color.y, color.z));
    if (maxValue > 1.f)
        return color /= maxValue;
    return color;
}

float Phong(float3 l, float3 n, float3 v, float ks, float e)
{
    float3 r = l - (n * 2.0f * dot(l, n));

    const float cos_a = max(dot(r, v), 0.0f);

    return ks * pow(cos_a, e);
}

float3 Lambert(float3 diffuse, float ks)
{
    return diffuse * ks / 3.141592653f;
}

float3x3 GetTangentMatrix(float3 tangent, float3 normal)
{
    float3 binormal = cross(normal, tangent);
    return float3x3(tangent, binormal, normal);
}

VS_OUTPUT VS(VS_INPUT input)
{
	VS_OUTPUT output = (VS_OUTPUT)0;
    
    float3 worldPosition = mul(float4(input.Position, 1.0f), gWorld).xyz;

    output.Position = mul(float4(input.Position, 1.0f), gWorldViewProj);
    output.WorldPosition = worldPosition;
    output.Color = input.Color;
    output.TexCoord = input.TexCoord;
    output.Normal = input.Normal;
    output.Tangent = input.Tangent;
    
    output.ViewDirection = normalize(worldPosition - gCameraOrigin);

	return output;
}

float4 PS(VS_OUTPUT input) : SV_TARGET 
{
    // gDiffuseMap.Sample(samPoint, input.TexCoord).rgb
    
    float3 worldNormal = mul(normalize(input.Normal), (float3x3) gWorld);
    float3 worldTangent = mul(normalize(input.Tangent), (float3x3) gWorld);
    
    float3x3 tangentSpaceAxis = GetTangentMatrix(worldTangent, worldNormal);
    
    float3 transformedNormal = mul(SampleNormal(input.TexCoord), tangentSpaceAxis);
    
    float observable_area = GetObservableArea(transformedNormal);
    
    float specular = gSpecularMap.Sample(gSamplerState, input.TexCoord).x;
    float gloss = gGlossMap.Sample(gSamplerState, input.TexCoord).x;
    
    float4 diffuse = gDiffuseMap.Sample(gSamplerState, input.TexCoord);
    
    return float4(MaxToOne(Lambert(diffuse.rgb, 7.0f) * observable_area + Phong(-gLightDirection, transformedNormal, input.ViewDirection, specular * 1.0f, gloss * 25.f)),
    diffuse.a);
}

technique11 DefaultTechnique
{
	pass P0
	{
        SetRasterizerState(gRasterizerState);
        SetDepthStencilState(gDepthStencilState, 0);
        SetBlendState(gBlendState, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetVertexShader( CompileShader( vs_5_0, VS() ) );
		SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, PS() ) );
	}
}