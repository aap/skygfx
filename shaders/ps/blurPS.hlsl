uniform sampler2D tex : register(s0);
uniform float3 pxSz : register(c0);

struct PS_INPUT
{
	float3 texcoord0	: TEXCOORD0;
};

float4
main(PS_INPUT IN) : COLOR
{
	float4 c = float4(0.0f, 0.0f, 0.0f, 0.0f);

	for(float i = 0; i < 10; i++){
		float2 uv = IN.texcoord0.xy + pxSz.xy*(i/9.0 - 0.5)*pxSz[2];
		c += tex2D(tex, uv);
	}
	c /= 10;
	c.a = 1.0f;
	return c;
}
