struct VS_INPUT 
{
	float3 Position : POSITION;
	float3 Color : COLOR;
    float2 Texcoord : TEXCOORD;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
};

struct VS_OUTPUT 
{
	float4 Position : SV_POSITION;
	float3 Color : COLOR;
};

float4x4 gWorldViewProj : WorldViewProjection;

VS_OUTPUT VS(VS_INPUT input) 
{
	VS_OUTPUT output = (VS_OUTPUT)0;

    output.Position = mul(float4(input.Position, 1.0f), gWorldViewProj);
    output.Color = input.Color;

	return output;
}

float4 PS(VS_OUTPUT input) : SV_TARGET 
{
    return float4(float3(1.0f, 1.0f, 1.0f), 1.0f);
}

technique11 DefaultTechnique
{
	pass P0
	{
		SetVertexShader( CompileShader( vs_5_0, VS() ) );
		SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, PS() ) );
	}
}