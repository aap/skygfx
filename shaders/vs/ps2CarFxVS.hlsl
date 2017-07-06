float4x4 World : register(c0);
float4x4 View : register(c4);
float4x4 Proj : register(c8);
float4x4 WorldIT : register(c12);

float3 sunDir : register(c23);

float3 reflData : register(c21);
float4 envXform : register(c22);
float envSwitch : register(c39);

struct VS_INPUT {
	float3 Position	: POSITION;
	float4 Normal	: NORMAL;
	float3 UV2	: TEXCOORD1;
};

struct VS_OUTPUT {
	float4 position		: POSITION;
	float3 texcoord1	: TEXCOORD0;
	float3 texcoord2	: TEXCOORD1;
	float4 envcolor		: COLOR0;
	float4 speccolor	: COLOR1;
};

VS_OUTPUT
main(VS_INPUT IN)
{	
	VS_OUTPUT OUT;
	float3 worldNormal = normalize(mul(IN.Normal, WorldIT).xyz);
	OUT.position = mul(Proj, mul(View, mul(World, float4(IN.Position, 1.0))));

	if (envSwitch == 1.0f) {		// env1 map
		OUT.texcoord1.xy = worldNormal.xy - envXform.xy;
		OUT.texcoord1.xy *= -envXform.zw;
	} else if (envSwitch == 2.0f) {		// env2 map ("x")
		float2 tmp = worldNormal.xy - envXform.xy;
		OUT.texcoord1.x = tmp.x + IN.UV2.x;
		OUT.texcoord1.y = tmp.y*envXform.y + IN.UV2.y;
		OUT.texcoord1.xy *= -envXform.zw;
	}
	OUT.texcoord1.z = 1.0;

	float3 N = mul((float3x3)View, worldNormal);
	float3 V = mul((float3x3)View, sunDir);
	float3 U = float3(V.x+1, V.y+1, V.z)*0.5;
	OUT.texcoord2.xyz = (U - N*dot(N, V));

	OUT.envcolor = float4(192.0, 192.0, 192.0, 0.0)/128.0*reflData.x*reflData.z;
	
	if (OUT.texcoord2.z < 0.0)
		OUT.speccolor = float4(96.0, 96.0, 96.0, 0.0)/128.0*reflData.y*reflData.z;
	else
		OUT.speccolor = float4(0.0, 0.0, 0.0, 0.0);
	return OUT;
}
