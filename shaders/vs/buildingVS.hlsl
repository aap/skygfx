struct VS_INPUT
{
	float4 Position		: POSITION;
	float3 Normal		: NORMAL;
	float2 TexCoord		: TEXCOORD0;
	float4 NightColor	: COLOR0;
	float4 DayColor		: COLOR1;
};

struct VS_OUTPUT {
	float4 position		: POSITION;
	float3 texcoord0	: TEXCOORD0;
	float4 color		: COLOR0;
};

float4x4	world : register(c0);
float4x4	view : register(c4);
float4x4	proj : register(c8);
float4		materialColor : register(c12);
float3		surfaceProps : register(c13);
float3		ambientLight : register(c14);
float4		shaderVars : register(c15);	// colorScale, nightMult, dayMult, reflSwitch
float4x4	texmat : register(c20);

VS_OUTPUT main(in VS_INPUT IN)
{
	VS_OUTPUT OUT;

	OUT.position = mul(proj, mul(view, mul(world, IN.Position)));
	OUT.texcoord0 = float3(0.0, 0.0, 0.0);
	OUT.texcoord0.xy = mul(texmat, float4(IN.TexCoord, 0.0, 1.0)).xy;

	OUT.color.rgb = (IN.DayColor*shaderVars[2] + IN.NightColor*shaderVars[1]).rgb;
	OUT.color.a = IN.NightColor.a;
	OUT.color *= materialColor / shaderVars[0];
	OUT.color.rgb += ambientLight*surfaceProps.x;

	return OUT;
}
