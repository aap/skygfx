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

float
scale(float low, float high, float x)
{
	return saturate(x/(high-low) - low/(high-low));
}

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

	//float low = 16/255.0;
	//float high = 235/255.0;
	//
	//c.r = scale(low, high, c.r);
	//c.g = scale(low, high, c.g);
	//c.b = scale(low, high, c.b);
	

//	float3 low = float3(16, 16, 16)/255.0;
//	float3 high = float3(240, 240, 240)/255.0;
//	c.r = lerp(low, high, c.r).r;
//	c.g = lerp(low, high, c.g).g;
//	c.b = lerp(low, high, c.b).b;

	//c = pow(abs(c), 1/2.2);
	return c;
}
