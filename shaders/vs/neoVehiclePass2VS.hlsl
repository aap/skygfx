struct VS_INPUT
{
	float4 Position		: POSITION;
	float3 Normal		: NORMAL;
	float2 TexCoord		: TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 position		: POSITION;
	float4 color		: COLOR0;
};


float4x4	combined	: register(c0);
float4x4	world		: register(c4);
float4x4	tex		: register(c8);
float3		eye		: register(c12);
float3		directDir	: register(c13);
float3		ambient		: register(c15);
float4		matCol		: register(c16);
float3		directCol	: register(c17);
float3		lightDir[6]	: register(c18);
float3		lightCol[6]	: register(c24);

float3		directSpec	: register(c30);
float4		reflProps	: register(c31);
float3		surfProps	: register(c32);

#define shininess (reflProps.x)
#define fresnel (reflProps.y)
#define lightmult (reflProps.z)
#define power (reflProps.w)
#define surfSpec (surfProps.y)

float
specTerm(float3 N, float3 L, float3 V, float pwr)
{
	return pow(saturate(dot(N, normalize(V + L))), pwr);
}

VS_OUTPUT
main(in VS_INPUT In)
{
	VS_OUTPUT Out;

	Out.position = mul(In.Position, combined);
	float3 V = normalize(eye - mul(In.Position, world).xyz);
	float3 N = mul(In.Normal, (float3x3)world).xyz;	// NORMAL MAT

	Out.color = float4(directSpec*specTerm(N, -directDir, V, power), 1.0);
	for(int i = 0; i < 6; i++)
		Out.color.rgb += lightCol[i]*specTerm(N, -lightDir[i], V, power*2);
	Out.color = saturate(Out.color*surfSpec*lightmult);

	return Out;
}
