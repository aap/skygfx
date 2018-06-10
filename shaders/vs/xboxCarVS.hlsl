float4x4 World : register(c0);
float4x4 View : register(c4);
float4x4 Proj : register(c8);
float4x4 WorldIT : register(c12);
float4x4 Texture : register(c16);

float4 matCol : register(c20);
float3 sunDir : register(c23);
float3 sunDiff : register(c24);
float3 sunAmb : register(c25);
float4 surfProps : register(c26);

float3 eye : register(c40);

#define surfAmb		(surfProps.x)
#define surfDiff	(surfProps.z)
#define specularity	(surfProps.y)

float envSwitch : register(c39);

struct VS_INPUT {
	float3 Position	: POSITION;
	float4 Normal	: NORMAL;
	float4 Color	: COLOR;
	float2 Texcoord0: TEXCOORD0;
	float2 Texcoord1: TEXCOORD1;
};

struct VS_OUTPUT {
	float4 Position		: POSITION;
	float2 Texcoord0	: TEXCOORD0;
	float2 Texcoord1	: TEXCOORD1;
	float4 Color		: COLOR0;
};

VS_OUTPUT
main(VS_INPUT IN)
{	
	VS_OUTPUT OUT;
	float3 N = normalize(mul(IN.Normal, WorldIT).xyz);
	OUT.Position = mul(Proj, mul(View, mul(World, float4(IN.Position, 1.0))));
	float3 V = normalize(eye - mul(World, float4(IN.Position, 1.0)).xyz);

	OUT.Texcoord0 = IN.Texcoord0;
	OUT.Texcoord1 = IN.Texcoord1;

	float3 specCol = float3(0.7, 0.7, 0.7);

	float3 diff = saturate(dot(N, -sunDir)) * sunDiff * surfDiff * matCol.xyz;
	float3 spec = pow(saturate(dot(N, normalize(V + -sunDir))), 16) * specCol * specularity;
	float3 amb = sunAmb * surfAmb * matCol.xyz;

	float3 eyeN = normalize(mul((float3x3)View, N));
	float2 tex1 = mul(Texture, float4(eyeN.x, eyeN.y, 1.0, 0.0)).xy;
	float2 tex2 = mul(Texture, float4(IN.Texcoord1.x, IN.Texcoord1.y, 1.0, 0.0)).xy;

	OUT.Color = float4(0.0, 0.0, 0.0, matCol.a);
	if(envSwitch == 1.0f){
		OUT.Color.rgb = amb + diff;
	}else if(envSwitch == 2.0f){
		OUT.Color.rgb = amb + diff;
		OUT.Texcoord1 = tex1;
	}else if(envSwitch == 3.0f){
		OUT.Color.rgb = amb + diff;
		OUT.Texcoord1 = tex2;
	}else if(envSwitch == 4.0f){
		OUT.Color.rgb = amb + (diff + spec)*0.5;
	}else if(envSwitch == 5.0f){
		OUT.Color.rgb = amb + (diff + spec)*0.5;
		OUT.Texcoord1 = tex1;
	}else if(envSwitch == 6.0f){
		OUT.Color.rgb = amb + (diff + spec)*0.5;
		OUT.Texcoord1 = tex2;
	}

	return OUT;
}
