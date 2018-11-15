uniform sampler2D tex : register(s0);
uniform float3 fogcol : register(c0);

float3		campos		: register(c1);
float2		fogdist		: register(c2);

struct PS_INPUT
{
	float3 texcoord0	: TEXCOORD0;
	float3 WorldPos		: TEXCOORD1;
	float4 color		: COLOR0;
//	float Fog		: COLOR1;
};

float4
main(PS_INPUT IN) : COLOR
{
	float3 ReflVector = IN.WorldPos - campos;
	float fog = clamp((fogdist.x - length(ReflVector)) / fogdist.y, 0.0, 1.0);
	float4 col = tex2D(tex, IN.texcoord0.xy)*IN.color;
	col.rgb = lerp(fogcol, col.rgb, fog);
	return col;
}
