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
	float3 texcoord2	: TEXCOORD2;
	float4 color		: COLOR0;
	float4 envcolor		: COLOR1;
	float4 speccolor	: TEXCOORD3;
};

VS_OUTPUT
mainVS(VS_INPUT IN)
{	
	VS_OUTPUT OUT;
	float3 worldNormal = normalize(mul(IN.Normal, WorldIT).xyz);
	OUT.position = mul(Proj, mul(View, mul(World, float4(IN.Position, 1.0))));

	OUT.texcoord0 = IN.UV;

	if (envSwitch == 0.0f) {		// PS2 style x-environment map
		float2 tmp = worldNormal.xy - envXform.xy;
		OUT.texcoord1.x = tmp.x + IN.UV2.x;
		OUT.texcoord1.y = tmp.y*envXform.y + IN.UV2.y;
		OUT.texcoord1.xy *= -envXform.zw;	// actually useless, doesn't matter
	} else if (envSwitch == 1.0f) {		// PC style x-environment map
		float4 intex = {IN.UV2.x, IN.UV2.y, 1.0, 0.0};
		OUT.texcoord1.xyz = mul(Texture, intex).xyz;
	} else if (envSwitch == 2.0f) {		// PS2 style environment map
		// assumes same as inverse transpose
		float4 camNormal;
		camNormal.xyz = normalize(mul((float3x3)View, mul((float3x3)World, IN.Normal.xyz)));
		camNormal.w = 0.0;
		OUT.texcoord1.xy = camNormal.xy - envXform.xy;
		OUT.texcoord1.xy *= -envXform.zw;
	} else {				// PC style environment map
		// assumes same as inverse transpose
		float4 camNormal;
		camNormal.xyz = normalize(mul((float3x3)View, mul((float3x3)World, IN.Normal.xyz)));
		camNormal.w = 0.0;
		OUT.texcoord1.xyz = mul(Texture, camNormal).xyz;
		OUT.texcoord1.xy /= OUT.texcoord1.z;
	}
	OUT.texcoord1.z = 1.0;

	// assumes same as inverse transpose
	float3 N = mul((float3x3)View, mul((float3x3)World, IN.Normal.xyz));
	float3 V = mul((float3x3)View, sunDir);
	float3 U = float3(V.x+1, V.y+1, V.z)*0.5;
	OUT.texcoord2.xyz = (U - N*dot(N, V));

	float l = max(0.0, dot(worldNormal, -sunDir));
	OUT.color = float4(IN.Color.rgb*surfProps.w, 1.0);
	OUT.color.xyz += sunAmb*surfProps.x;
	OUT.color.xyz += l*sunDiff*surfProps.z;
	for(int i = 0; i < activeLights; i++) {
		l = max(0.0, dot(worldNormal, -lights[i].dir));
		OUT.color.xyz += l*lights[i].diff*surfProps.z;
	}
	OUT.color *= matCol;

	OUT.envcolor = float4(192.0, 192.0, 192.0, 0.0)/128.0*reflData.x*reflData.z;

	if (OUT.texcoord2.z < 0.0)
		OUT.speccolor = float4(96.0, 96.0, 96.0, 0.0)/128.0*reflData.y*reflData.z;
	else
		OUT.speccolor = float4(0.0, 0.0, 0.0, 0.0);
	return OUT;
}

float4
mainPS(VS_OUTPUT IN) : COLOR
{
	float4 result = tex2D(tex0, IN.texcoord0.xy) * IN.color;
	result.xyz += (tex2D(tex1, IN.texcoord1.xy) * IN.envcolor).xyz;
	result.xyz += (tex2D(tex2, IN.texcoord2.xy) * IN.speccolor).xyz;
	return result;
}
