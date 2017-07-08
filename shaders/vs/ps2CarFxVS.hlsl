float4x4	combined	: register(c0);
float3		ambient		: register(c4);
float3		directCol[7]	: register(c5);
float3		directDir[7]	: register(c12);
float4		matCol		: register(c19);
float4		surfProps	: register(c20);

float4		fxParams	: register(c30);
float4		envXform	: register(c31);
float3x3	envmat		: register(c32);
float3x3	specmat		: register(c35);
float3		lightdir	: register(c38);

#define fxSwitch (fxParams.x)
#define shininess (fxParams.y)
#define specularity (fxParams.z)
#define lightmult (fxParams.w)

struct VS_INPUT {
	float4 Position	: POSITION;
	float3 Normal	: NORMAL;
	float2 Texcoord1: TEXCOORD1;
};

struct VS_OUTPUT {
	float4 Position		: POSITION;
	float2 Texcoord0	: TEXCOORD0;
	float3 Texcoord1	: TEXCOORD1;
	float4 Envcolor		: COLOR0;
	float4 Speccolor	: COLOR1;
};

VS_OUTPUT
main(VS_INPUT IN)
{	
	VS_OUTPUT OUT;
	OUT.Position = mul(IN.Position, combined);

	float3 envNormal = mul(envmat, IN.Normal);
	if(fxSwitch == 1.0f){		// env1 map
		OUT.Texcoord0.xy = envNormal.xy - envXform.xy;
		OUT.Texcoord0.xy *= -envXform.zw;
	}else if(fxSwitch == 2.0f){		// env2 map ("x")
		OUT.Texcoord0 = envNormal.xy - envXform.xy;
		OUT.Texcoord0.y *= envXform.y;
		OUT.Texcoord0.xy += IN.Texcoord1;
		OUT.Texcoord0.xy *= -envXform.zw;
	}
	OUT.Envcolor = float4(192.0, 192.0, 192.0, 0.0)/128.0*shininess*lightmult;

	float3 N = mul(specmat, IN.Normal);
	float3 U = (lightdir + float3(1.0, 1.0, 0.0))*0.5;
	OUT.Texcoord1.xyz = U - N*dot(N, lightdir);

	if (OUT.Texcoord1.z < 0.0)
		OUT.Speccolor = float4(96.0, 96.0, 96.0, 0.0)/128.0*specularity*lightmult;
	else
		OUT.Speccolor = float4(0.0, 0.0, 0.0, 0.0);

	return OUT;
}
