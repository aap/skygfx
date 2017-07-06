sampler2D tex1 : register(s1);

struct VS_OUTPUT {
	float4 position		: POSITION;
	float3 texcoord1	: TEXCOORD0;
	float4 envcolor		: COLOR0;
	float4 speccolor	: COLOR1;
};

float4
main(VS_OUTPUT IN) : COLOR
{
	return tex2D(tex1, IN.texcoord1.xy) * IN.envcolor + IN.speccolor;
}
