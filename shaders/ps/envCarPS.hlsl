uniform sampler2D tex0 : register(s0);
uniform sampler2D tex1 : register(s1);
uniform sampler2D tex2 : register(s2);

float3		eye		: register(c0);
float3		sunDir		: register(c1);

struct PS_INPUT
{
	float3 texcoord0	: TEXCOORD0;
	float3 texcoord1	: TEXCOORD1;
	float4 color		: COLOR0;
	float4 envColor		: COLOR1;
};

float4
main(PS_INPUT IN) : COLOR
{
	float2 ReflPos = normalize(IN.texcoord1.xy) * (IN.texcoord1.z*0.5 + 0.5);
	ReflPos = ReflPos*float2(0.5, -0.5) + float2(0.5, 0.5);
	float4 env = tex2D(tex1, ReflPos);

	float4 diff = tex2D(tex0, IN.texcoord0.xy);
//	float4 env = tex2D(tex1, IN.texcoord1.xy);
	float4 mask = tex2D(tex2, IN.texcoord1.xy*0.5 + 0.5);

	float4 diffpass = diff*IN.color;
	float4 envpass = env*mask;;
	float4 final = lerp(diffpass, envpass, IN.envColor.r) + IN.envColor.g;
//	final.rgb = IN.envColor.r;
	final.a = diffpass.a;
	return final;
}
