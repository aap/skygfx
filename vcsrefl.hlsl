// compile: fxc /T vs_2_a /E mainVS /Fh vs.txt ps2refl.hlsl
//          fxc /T ps_2_a /E mainPS /Fh ps.txt ps2refl.hlsl
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

struct Directional {
	float3 dir;
	float3 diff;
};
#define MAX_LIGHTS 6
int activeLights : register(i0);
Directional lights[MAX_LIGHTS] : register(c27);

sampler2D tex0 : register(s0);
sampler2D tex1 : register(s1);

struct VS_INPUT {
	float3 Position	: POSITION;
	float4 Normal	: NORMAL;
	float4 Color	: COLOR;
	float3 UV	: TEXCOORD0;
};

struct VS_OUTPUT {
	float4 position		: POSITION;
	float3 texcoord0	: TEXCOORD0;
	float3 texcoord1	: TEXCOORD1;
	float4 color		: COLOR0;
	float4 reflamount	: COLOR1;
};

VS_OUTPUT
mainVS(VS_INPUT IN)
{	
	VS_OUTPUT OUT;
	float3 worldNormal = normalize(mul(IN.Normal, WorldIT).xyz);
	OUT.position = mul(Proj, mul(View, mul(World, float4(IN.Position, 1.0))));

	OUT.texcoord0 = IN.UV;

	float3 camNormal, camVertex;
	camNormal = normalize(mul((float3x3)View, worldNormal));
	camVertex = normalize(mul(View, mul(World, float4(IN.Position, 1.0))).xyz);
	float3 r = reflect(-camVertex, camNormal);
	float m = 2.0 * sqrt(r.x*r.x + r.y*r.y + (r.z+1.0)*(r.z+1.0));
	OUT.texcoord1.x = r.x/m + 0.5;
	OUT.texcoord1.y = r.y/m + 0.5;
	OUT.texcoord1.z = 1.0;

	float l = max(0.0, dot(worldNormal, -sunDir));
	OUT.color = float4(IN.Color.rgb*surfProps.w, 1.0);
	OUT.color.xyz += sunAmb*surfProps.x;
	OUT.color.xyz += l*sunDiff*surfProps.z;
	for(int i = 0; i < activeLights; i++) {
		l = max(0.0, dot(worldNormal, -lights[i].dir));
		OUT.color.xyz += l*lights[i].diff*surfProps.z;
	}
	OUT.color *= matCol;

	OUT.reflamount = float4(reflData.x, reflData.x, reflData.x, 0.0);

	return OUT;
}

float4
mainPS(VS_OUTPUT IN) : COLOR
{
	float4 result = tex2D(tex0, IN.texcoord0.xy) * IN.color;
//	IN.reflamount = float4(0x26, 0x26, 0x26, 0)/255.0f;
	result.xyz = (result*(float4(1,1,1,1)-IN.reflamount)).xyz + (tex2D(tex1, IN.texcoord1.xy)*IN.reflamount).xyz;
	return result;
}
