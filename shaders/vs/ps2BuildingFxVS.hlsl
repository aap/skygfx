float4x4	combined : register(c0);

float4		fxParams	: register(c36);
float4		envXform	: register(c37);
float3x3	envmat		: register(c38);

#define shininess (fxParams.x)
#define lightmult (fxParams.y)


struct VS_INPUT {
	float4 Position		: POSITION;
	float3 Normal		: NORMAL;
};

struct VS_OUTPUT {
	float4 Position		: POSITION;
	float2 Texcoord0	: TEXCOORD0;
	float4 Envcolor		: COLOR0;
};

VS_OUTPUT
main(VS_INPUT IN)
{	
	VS_OUTPUT OUT;
	OUT.Position = mul(IN.Position, combined);

	OUT.Texcoord0 = mul(envmat, IN.Normal).xy - envXform.xy;
	OUT.Texcoord0 *= -envXform.zw;

	OUT.Envcolor = float4(192.0, 192.0, 192.0, 0.0)/128.0*shininess*lightmult;

	return OUT;
}
