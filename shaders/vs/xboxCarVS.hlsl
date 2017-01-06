float4x4 World : register(c0);
float4x4 View : register(c4);
float4x4 Proj : register(c8);
float4x4 WorldIT : register(c12);
float4x4 Texture : register(c16);

float4 matCol : register(c20);
float3 reflData : register(c21);
float4 envXform : register(c22);
float3 sunDir : register(c23);
float3 sunDiff : register(c24);
float3 sunAmb : register(c25);
float4 surfProps : register(c26);

float4 eye : register(c40);

struct Directional {
	float3 dir;
	float3 diff;
};
#define MAX_LIGHTS 6
int activeLights : register(i0);
Directional lights[MAX_LIGHTS] : register(c27);

float envSwitch : register(c39);

sampler2D tex0 : register(s0);
sampler2D tex1 : register(s1);
sampler2D tex2 : register(s2);

struct VS_INPUT {
	float3 Position	: POSITION;
	float4 Normal	: NORMAL;
	float4 Color	: COLOR;
	float3 UV	: TEXCOORD0;
	float3 UV2	: TEXCOORD1;
};

struct VS_OUTPUT {
	float4 position		: POSITION;
	float3 texcoord0	: TEXCOORD0;
	float3 texcoord1	: TEXCOORD1;
	float4 color		: COLOR0;
};

VS_OUTPUT
main(VS_INPUT IN)
{	
	VS_OUTPUT OUT;
	float3 N = normalize(mul(IN.Normal, WorldIT).xyz);
	OUT.position = mul(Proj, mul(View, mul(World, float4(IN.Position, 1.0))));
	float3 V = normalize(eye - mul(World, float4(IN.Position, 1.0)).xyz);

	OUT.texcoord0 = IN.UV;
	OUT.texcoord1 = IN.UV2;

	float3 specCol = float3(0.7, 0.7, 0.7);

	float3 diff = saturate(dot(N, -sunDir)) * sunDiff * surfProps.z*matCol;
	float3 spec = pow(saturate(dot(N, normalize(V + -sunDir))), 16) * specCol * reflData.y;
	float3 amb = sunAmb * surfProps.x*matCol;

	float3 tex1 = mul(Texture, float4(IN.UV2.x, IN.UV2.y, 1.0, 0.0)).xyz;
	float3 eyeN = normalize(mul((float3x3)View, N));
	float3 tex2 = mul(Texture, float4(eyeN.x, eyeN.y, 1.0, 0.0)).xyz;

	OUT.color = float4(0.0, 0.0, 0.0, matCol.a);
	if(envSwitch == 1.0f){
		OUT.color.rgb = amb + diff;
	}else if(envSwitch == 2.0f){
		OUT.color.rgb = amb + diff;
		OUT.texcoord1.xyz = tex2;
	}else if(envSwitch == 3.0f){
		OUT.color.rgb = amb + diff;
		OUT.texcoord1.xyz = tex1;
	}else if(envSwitch == 4.0f){
		OUT.color.rgb = amb + (diff + spec)*0.5;
	}else if(envSwitch == 5.0f){
		OUT.color.rgb = amb + (diff + spec)*0.5;
		OUT.texcoord1.xyz = tex2;
	}else if(envSwitch == 6.0f){
		OUT.color.rgb = amb + (diff + spec)*0.5;
		OUT.texcoord1.xyz = tex1;
	}

	return OUT;
}
