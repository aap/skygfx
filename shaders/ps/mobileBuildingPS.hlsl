uniform sampler2D tex0 : register(s0);
uniform sampler2D tex1 : register(s1);
uniform sampler2D tex2 : register(s2);

uniform float detailtile : register(c1);

struct PS_INPUT
{
	float2 texcoord0	: TEXCOORD0;
	float2 texcoord1	: TEXCOORD1;
	float4 color		: COLOR0;
};

float4
main(PS_INPUT IN) : COLOR
{
	float4 final = tex2D(tex0, IN.texcoord0)*IN.color;
	final.rgb *= tex2D(tex2, IN.texcoord0*detailtile).rgb*2.0;
	return final;
}
