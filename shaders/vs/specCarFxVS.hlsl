float4x4 World : register(c0);
float4x4 View : register(c4);
float4x4 Proj : register(c8);
float4x4 WorldIT : register(c12);

float3 sunDir : register(c23);

float3 reflData : register(c21);
float4 envXform : register(c22);
float envSwitch : register(c39);

float3 eye : register(c40);

sampler2D tex1 : register(s1);
sampler2D tex2 : register(s2);

struct VS_INPUT {
	float3 Position	: POSITION;
	float4 Normal	: NORMAL;
	float3 UV2	: TEXCOORD1;
};

struct VS_OUTPUT {
	float4 position		: POSITION;
	float3 texcoord1	: TEXCOORD0;
	float4 envcolor		: COLOR0;
	float4 speccolor	: COLOR1;
};

VS_OUTPUT
main(VS_INPUT IN)
{	
	VS_OUTPUT OUT;
	OUT.position = mul(Proj, mul(View, mul(World, float4(IN.Position, 1.0))));
	float3 N = normalize(mul(IN.Normal, WorldIT).xyz);
	float3 V = normalize(eye - mul(World, float4(IN.Position, 1.0)).xyz);

	float3 specCol = float3(0.7, 0.7, 0.7);
	OUT.speccolor.rgb = pow(saturate(dot(N, normalize(V + -sunDir))), 16) * specCol * reflData.y;
	OUT.speccolor.a = 1.0;

	if(envSwitch == 0.0f){		// PS2 style x-environment map
		float2 tmp = N.xy - envXform.xy;
		OUT.texcoord1.x = tmp.x + IN.UV2.x;
		OUT.texcoord1.y = tmp.y*envXform.y + IN.UV2.y;
		OUT.texcoord1.xy *= -envXform.zw;	// actually useless, doesn't matter
	}else{				// PS2 style environment map
		OUT.texcoord1.xy = N.xy - envXform.xy;
		OUT.texcoord1.xy *= -envXform.zw;
	}
	OUT.texcoord1.z = 1.0;

	OUT.envcolor = float4(192.0, 192.0, 192.0, 0.0)/128.0*reflData.x*reflData.z;
	return OUT;
}
