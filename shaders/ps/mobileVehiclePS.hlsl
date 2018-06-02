uniform sampler2D tex0 : register(s0);
uniform sampler2D tex1 : register(s1);

float4		fxParams	: register(c30);

#define fxSwitch (fxParams.x)
#define shininess (fxParams.y)
#define specularity (fxParams.z)
#define lightmult (fxParams.w)

struct PS_INPUT
{
	float2 texcoord0	: TEXCOORD0;
	float3 texcoord1	: TEXCOORD1;
	float4 color		: COLOR0;
	float3 spec		: COLOR1;
};

float4
main(PS_INPUT IN) : COLOR
{
	float4 col = tex2D(tex0, IN.texcoord0)*IN.color;

	float2 ReflPos = normalize(IN.texcoord1.xy) * (IN.texcoord1.z*0.5 + 0.5);
	ReflPos = ReflPos*float2(0.5, -0.5) + float2(0.5, 0.5);
	float4 ReflCol = tex2D(tex1, ReflPos);
	col.rgb = lerp(col.rgb, ReflCol.rgb, shininess);
	col.a += ReflCol.b * 0.125;

	col.rgb += IN.spec;

	return col;
}
