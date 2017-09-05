uniform sampler2D tex : register(s0);
uniform float4 redGrade : register(c0);
uniform float4 greenGrade : register(c1);
uniform float4 blueGrade : register(c2);

uniform float3 contrastMult : register(c3);
uniform float3 contrastAdd : register(c4);

struct PS_INPUT
{
	float3 texcoord0	: TEXCOORD0;
};

float4
main(PS_INPUT IN) : COLOR
{
	float4 c = tex2D(tex, IN.texcoord0.xy);
	c.a = 1.0f;
	float4 o;

	//c.rgb = c.rgb*contrastMult + contrastAdd;
	//return c;

	o.r = dot(redGrade, c);
	o.g = dot(greenGrade, c);
	o.b = dot(blueGrade, c);
	o.a = 1.0f;
	return o;
}
