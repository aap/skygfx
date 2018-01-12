float4x4	combined	: register(c0);
float3		ambient		: register(c4);
float3		directCol[7]	: register(c5);
float3		directDir[7]	: register(c12);
float4		matCol		: register(c19);
float4		surfProps	: register(c20);

#define surfAmb		(surfProps.x)
#define surfDiff	(surfProps.z)
#define isPrelit	(surfProps.w)

struct VS_INPUT {
	float4 Position	: POSITION;
	float3 Normal	: NORMAL;
	float4 Color	: COLOR;
	float2 Texcoord0: TEXCOORD0;
};

struct VS_OUTPUT {
	float4 Position		: POSITION;
	float2 Texcoord0	: TEXCOORD0;
	float4 Color		: COLOR0;
};

VS_OUTPUT
main(VS_INPUT IN)
{	
	VS_OUTPUT OUT;
	OUT.Position = mul(IN.Position, combined);
	OUT.Texcoord0 = IN.Texcoord0;

	OUT.Color = float4(IN.Color.rgb*isPrelit, 1.0);
	OUT.Color.xyz += ambient*surfAmb;
	for(int i = 0; i < 7; i++){
		float l = max(0.0, dot(IN.Normal, -directDir[i]));
		OUT.Color.xyz += l*directCol[i]*surfDiff;
	}
	OUT.Color = saturate(OUT.Color)*matCol;

	return OUT;
}
