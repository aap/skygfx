uniform sampler2D tex : register(s0);
uniform float4 colorscale : register(c0);

struct PS_INPUT
{
	float3 texcoord0	: TEXCOORD0;
	float4 color		: COLOR0;
};

float4
main(PS_INPUT IN) : COLOR
{
	float4 c = tex2D(tex, IN.texcoord0);
	return c*IN.color*colorscale.x;
}
