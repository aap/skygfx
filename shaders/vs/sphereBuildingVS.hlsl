float4x4	combined	: register(c0);
float3		ambient		: register(c4);
float4		matCol		: register(c19);
float3		surfProps	: register(c20);

float4		shaderParams	: register(c29);
float4		dayparam	: register(c30);
float4		nightparam	: register(c31);
float4x4	texmat		: register(c32);
float3x3	envmat		: register(c38);

float3		campos		: register(c44);

#define colorScale (shaderParams.x)
#define surfAmb (surfProps.x)

struct VS_INPUT
{
	float4 Position		: POSITION;
	float2 TexCoord		: TEXCOORD0;
	float4 NightColor	: COLOR0;
	float4 DayColor		: COLOR1;
};

struct VS_OUTPUT {
	float4 Position		: POSITION;
	float2 Texcoord0	: TEXCOORD0;
	float4 Color		: COLOR0;
};

VS_OUTPUT main(in VS_INPUT IN)
{
	VS_OUTPUT OUT;

	// Sphere map
	// 'combined' is world matrix
	float4 WorldPos = mul(IN.Position, combined);
	float3 ReflVector = WorldPos.xyz - campos;
	float3 ReflPos = normalize(ReflVector);
	ReflPos.xy = normalize(ReflPos.xy) * (ReflPos.z * 0.5 + 0.5);
	OUT.Position = float4(ReflPos.xy, length(ReflVector)*0.002, 1.0);

	OUT.Texcoord0 = mul(texmat, float4(IN.TexCoord, 0.0, 1.0)).xy;

	OUT.Color = IN.DayColor*dayparam + IN.NightColor*nightparam;
	OUT.Color.rgb += ambient*surfAmb;
	OUT.Color *= matCol;

	return OUT;
}
