uniform sampler2D tex : register(s0);
uniform float3 fx : register(c0);
uniform float4 xform : register(c1);
#define limit (fx.x)
#define intensity (fx.y)
#define passes (fx.z)

struct PS_INPUT
{
	float3 texcoord0	: TEXCOORD0;
};

float4
main(PS_INPUT IN) : COLOR
{
	float2 uv = IN.texcoord0.xy;

//	uv = (uv - xform.xy) * xform.zw + xform.xy;
	uv = uv*xform.zw + xform.xy;

	float4 c = tex2D(tex, uv);

	c = saturate(c*2.0 - float4(1,1,1,1)*limit);
	c.a = intensity*passes;

	return c;
}
