float4x4	combined	: register(c0);
float3		ambient		: register(c4);
float3		directCol[7]	: register(c5);
float3		directDir[7]	: register(c12);
float4		matCol		: register(c19);
float4		surfProps	: register(c20);
float4		fxParams	: register(c21);

float4x4	world		: register(c30);
float3		eye		: register(c34);

#define fresnel		(fxParams.x)
#define power		(fxParams.y)
#define lightmult	(fxParams.z)

#define surfAmb		(surfProps.x)
#define surfDiff	(surfProps.y)
#define surfSpec	(surfProps.z)
#define surfRefl	(surfProps.w)

struct VS_INPUT {
	float4 Position	: POSITION;
	float3 Normal	: NORMAL;
	float4 Color	: COLOR;
	float2 Texcoord0: TEXCOORD0;
};

struct VS_OUTPUT {
	float4 Position		: POSITION;
	float2 Texcoord0	: TEXCOORD0;
	float3 Texcoord1	: TEXCOORD1;
	float4 Color		: COLOR0;
	float4 EnvColor		: COLOR1;
};

VS_OUTPUT
main(VS_INPUT IN)
{	
	VS_OUTPUT OUT;
	OUT.Position = mul(IN.Position, combined);
	OUT.Texcoord0 = IN.Texcoord0;

	OUT.Color = float4(0.0, 0.0, 0.0, 1.0);
	OUT.Color.xyz += ambient*surfAmb;
	for(int i = 0; i < 7; i++){
		float l = max(0.0, dot(IN.Normal, -directDir[i]));
		OUT.Color.xyz += l*directCol[i]*surfDiff;
	}
	OUT.Color = saturate(OUT.Color)*matCol;

	// Sphere map
	float4 WorldPos = mul(IN.Position, world);
	float3 WorldNormal = normalize(mul(IN.Normal, (float3x3)world));
	float3 ViewVector = normalize(WorldPos.xyz - eye);
	float3 ReflVector = ViewVector - 2.0*dot(ViewVector, WorldNormal)*WorldNormal;
	OUT.Texcoord1 = ReflVector;

	// Specular
	float specAmt = pow(max(dot(ReflVector, mul(-directDir[0], (float3x3)world)), 0.0), power);
	OUT.EnvColor.g = specAmt*surfSpec*lightmult;

	// reflection intensity
	float b = 1.0 - saturate(dot(-ViewVector, WorldNormal));
	OUT.EnvColor.r = lerp(b*b*b*b*b, 1.0f, fresnel)*surfRefl*lightmult;

	OUT.EnvColor.b = 0.0;
	OUT.EnvColor.a = 0.0;

	return OUT;
}
