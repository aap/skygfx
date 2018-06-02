uniform sampler2D tex : register(s0);
uniform float3 contrastMult : register(c3);
uniform float3 contrastAdd : register(c4);

struct PS_INPUT
{
	float3 texcoord0	: TEXCOORD0;
};

float4
main(PS_INPUT IN) : COLOR
{
	float4 c = tex2D(tex, IN.texcoord0.xy);
	c.a = 1.0f;
	c.rgb = c.rgb*contrastMult + contrastAdd;
	return c;
}
