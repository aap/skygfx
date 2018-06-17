uniform sampler2D tex0 : register(s0);
uniform sampler2D tex1 : register(s1);

struct PS_INPUT
{
	float2 texcoord0	: TEXCOORD0;
	float2 texcoord1	: TEXCOORD1;
	float4 color		: COLOR0;
	float4 envcolor		: COLOR1;
};

float4
main(PS_INPUT IN) : COLOR
{
	return tex2D(tex0, IN.texcoord0)*IN.color +
	       tex2D(tex1, IN.texcoord1)*IN.envcolor;
}
