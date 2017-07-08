float4x4	combined	: register(c0);
float3		ambient		: register(c4);
float3		directCol[7]	: register(c5);
float3		directDir[7]	: register(c12);
float4		matCol		: register(c19);
float4		surfProps	: register(c20);

float4		fxParams	: register(c30);
float4		envXform	: register(c31);
float3x3	envmat		: register(c32);
float3		eye		: register(c35);

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
	float4 Envcolor		: COLOR0;
	float4 Speccolor	: COLOR1;
};

VS_OUTPUT
main(VS_INPUT IN)
{
	VS_OUTPUT OUT;
	float3 envNormal = mul((float3x3)envmat, IN.Normal);
	OUT.Position = mul(IN.Position, combined);

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

	float3 V = normalize(eye - IN.Position.xyz);
	float spec = pow(saturate(dot(IN.Normal, normalize(V + -directDir[0]))), 16);
	OUT.Speccolor.rgb = spec*3 * float3(0.75, 0.75, 0.75)*specularity*lightmult;
	OUT.Speccolor.a = 1.0;

	/* to simulate ps2 specdot:
		OUT.Speccolor.r = spec;
		OUT.Speccolor.g = specularity;
		OUT.Speccolor.b = lightmult;
		OUT.Speccolor.a = 0.0;
	*/

	return OUT;
}
