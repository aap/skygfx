float4 main(uniform sampler2D tex : register(s0),

            in float2 Tex0 : TEXCOORD0) : COLOR0
{
	float4 color;
	float4 c = tex2D(tex, Tex0);
	color.rgb = 1.0f;
	color.a = 2*saturate(c.a);
	return color;
}
