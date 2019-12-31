float4x4	combined	: register(c0);
float3		ambient		: register(c4);
float3		directCol[7]	: register(c5);
float3		directDir[7]	: register(c12);
float4		matCol		: register(c19);
float4		surfProps	: register(c20);
float4		fxParams	: register(c21);

float4x4	world		: register(c30);
float3		eye		: register(c34);

// spec test
float3x3	specmat		: register(c40);

#define fresnel		(fxParams.x)
//#define power		(fxParams.y)
//#define lightmult	(fxParams.z)
#define shininess	(fxParams.w)

#define surfAmb		(surfProps.x)
#define surfDiff	(surfProps.y)
//#define surfSpec	(surfProps.z)
#define surfPrelight	(surfProps.w)

struct VS_INPUT {
	float4 Position	: POSITION;
	float3 Normal	: NORMAL;
	float4 Color	: COLOR;
	float2 Texcoord0: TEXCOORD0;
};

struct VS_OUTPUT {
	float4 Position		: POSITION;
	float2 Texcoord0	: TEXCOORD0;
	float3 WorldNormal	: TEXCOORD1;
	float3 WorldPos		: TEXCOORD2;
	float4 Color		: COLOR0;
	float4 EnvColor		: COLOR1;
};

VS_OUTPUT
main(VS_INPUT IN)
{	
	VS_OUTPUT OUT;
	OUT.Position = mul(IN.Position, combined);
	OUT.Texcoord0 = IN.Texcoord0;

	OUT.Color = float4(IN.Color.rgb*surfPrelight, 1.0);
	OUT.Color.xyz += ambient*surfAmb;
	for(int i = 0; i < 7; i++){
		float l = max(0.0, dot(IN.Normal, -directDir[i]));
		OUT.Color.xyz += l*directCol[i]*surfDiff;
	}
	OUT.Color = saturate(OUT.Color)*matCol;

	float4 WorldPos = mul(IN.Position, world);
	float3 WorldNormal = normalize(mul(IN.Normal, (float3x3)world));
	float3 ViewVector = normalize(WorldPos.xyz - eye);
	OUT.WorldPos = WorldPos;
	OUT.WorldNormal = WorldNormal;

	// reflection intensity
	float b = 1.0 - saturate(dot(-ViewVector, WorldNormal));
	OUT.EnvColor = lerp(1.0f, b*b*b*b*b, fresnel)*shininess;

	return OUT;
}
