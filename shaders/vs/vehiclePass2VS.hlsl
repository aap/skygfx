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


float4x4    combined    : register(c0);
float4x4    world       : register(c4);
float4x4    tex         : register(c8);
float3      eye         : register(c12);
float3      directDir   : register(c13);
float3      ambient     : register(c15);
float4	    matCol      : register(c16);
float3      directCol   : register(c17);
float3      lightDir[4] : register(c18);
float3      lightCol[4] : register(c22);

float3	    directSpec  : register(c26);
float4	    reflProps   : register(c27);

float
specTerm(float3 N, float3 L, float3 V, float power)
{
	return pow(saturate(dot(N, normalize(V + L))), power);
}

VS_OUTPUT
main(in VS_INPUT In)
{
	VS_OUTPUT Out;

	Out.position = mul(In.Position, combined);
	float3 V = normalize(eye - mul(In.Position, world).xyz);
	float3 N = mul(In.Normal, (float3x3)world).xyz;	// NORMAL MAT

	Out.color = float4(directSpec*specTerm(N, -directDir, V, reflProps.w), 1.0);
	for(int i = 0; i < 4; i++)
		Out.color.rgb += lightCol[i]*specTerm(N, -lightDir[i], V, reflProps.w*2);
	Out.color = saturate(Out.color);

	return Out;
}
