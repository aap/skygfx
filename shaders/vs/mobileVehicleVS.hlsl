float4x4	combined	: register(c0);
float3		ambient		: register(c4);
float3		directCol[7]	: register(c5);
float3		directDir[7]	: register(c12);
float4		matCol		: register(c19);
float4		surfProps	: register(c20);

float4		fxParams	: register(c30);
float4x4	worldmat	: register(c31);
float3		campos		: register(c35);

#define surfAmb		(surfProps.x)
#define surfDiff	(surfProps.z)
#define isPrelit	(surfProps.w)

#define fxSwitch (fxParams.x)
#define shininess (fxParams.y)
#define specularity (fxParams.z)
#define lightmult (fxParams.w)

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
	float3 Spec		: COLOR1;
};

VS_OUTPUT
main(VS_INPUT IN)
{	
	VS_OUTPUT OUT;
	OUT.Position = mul(IN.Position, combined);
	OUT.Texcoord0 = IN.Texcoord0;

	float4 WorldPos = mul(IN.Position, worldmat);
	float3 WorldNormal = mul(IN.Normal, (float3x3)worldmat);
	float3 ReflVector = normalize(WorldPos.xyz - campos);
	ReflVector = ReflVector - 2.0*dot(ReflVector, WorldNormal)*WorldNormal;
	OUT.Texcoord1 = ReflVector;
	// for the other env map
//	OUT.Texcoord1 = float2(length(ReflVector.xy), ReflVector.z*0.5 + 0.25);

	float specAmt = pow(max(dot(ReflVector, mul(-directDir[0], (float3x3)worldmat)), 0.0), 20.0f) * specularity * 2.0;
	OUT.Spec = specAmt*directCol[0];

	OUT.Color = float4(IN.Color.rgb*isPrelit, 1.0);
	OUT.Color.xyz += ambient*surfAmb;
	for(int i = 0; i < 7; i++){
		float l = max(0.0, dot(IN.Normal, -directDir[i]));
		OUT.Color.xyz += l*directCol[i]*surfDiff;
	}
	OUT.Color = saturate(OUT.Color)*matCol;

	return OUT;
}
