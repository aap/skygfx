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

/* simulate ps2 specdot texture:
	float4 ret;
	float s = IN.speccolor.r;
	if(s < 0.05) s = 0.0;
	else s = 1.0;
	ret.rgb = s * float3(0.75, 0.75, 0.75)*IN.speccolor.g*IN.speccolor.b;
	ret.a = 1.0;
	return ret;
*/
}
