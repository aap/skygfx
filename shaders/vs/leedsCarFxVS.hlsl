float4x4	combined	: register(c0);
float4		fxParams	: register(c30);
float3x3	envmat		: register(c32);
float4x4	texmat		: register(c36);

#define shininess (fxParams.y)
#define lightmult (fxParams.w)

struct VS_INPUT {
	float4 Position	: POSITION;
	float3 Normal	: NORMAL;
	float2 Texcoord1: TEXCOORD1;
};

struct VS_OUTPUT {
	float4 Position	: POSITION;
	float2 Texcoord	: TEXCOORD0;
	float4 Color	: COLOR0;
};

VS_OUTPUT
main(VS_INPUT IN)
{	
	VS_OUTPUT OUT;
	OUT.Position = mul(IN.Position, combined);

	float3 envNormal = mul(envmat, IN.Normal);
	OUT.Texcoord = mul(texmat, float4(envNormal.xy, 0.0, 1.0)).xy;
	OUT.Color = float4(128.0, 128.0, 128.0, 255.0)/255.0*shininess*lightmult;
	OUT.Color.a = 1.0;
	return OUT;
}
