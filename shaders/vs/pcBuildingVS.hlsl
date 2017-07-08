float4x4	combined	: register(c0);
float3		ambient		: register(c4);
float4		matCol		: register(c19);
float3		surfProps	: register(c20);

float4		shaderParams	: register(c29);
float4		dayparam	: register(c30);
float4		nightparam	: register(c31);
float4x4	texmat		: register(c32);
float3x3	envmat		: register(c38);

#define colorScale (shaderParams.x)
#define surfAmb (surfProps.x)

struct VS_INPUT
{
	float4 Position		: POSITION;
	float3 Normal		: NORMAL;
	float2 TexCoord		: TEXCOORD0;
	float4 NightColor	: COLOR0;
	float4 DayColor		: COLOR1;
};

struct VS_OUTPUT {
	float4 Position		: POSITION;
	float2 Texcoord0	: TEXCOORD0;
	float2 Texcoord1	: TEXCOORD1;
	float4 Color		: COLOR0;
};

VS_OUTPUT main(in VS_INPUT IN)
{
	VS_OUTPUT OUT;

	OUT.Position = mul(IN.Position, combined);
	OUT.Texcoord0 = mul(texmat, float4(IN.TexCoord, 0.0, 1.0)).xy;
	OUT.Texcoord1 = mul(envmat, IN.Normal).xy;

	OUT.Color = IN.DayColor*dayparam + IN.NightColor*nightparam;
	OUT.Color.rgb += ambient*surfAmb;
	OUT.Color *= matCol;

	return OUT;
}
