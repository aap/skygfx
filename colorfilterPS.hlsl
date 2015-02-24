float4 main(uniform sampler2D Diffuse : register(s0),
            uniform float4 RGB1 : register(c0),
            uniform float4 RGB2 : register(c1),

            in float2 Tex0 : TEXCOORD0,
            in float2 Tex1 : TEXCOORD1) : COLOR0
{
	float4 color = tex2D(Diffuse, Tex0);
	float4 color2 = tex2D(Diffuse, Tex1);

	float3 c1 = saturate(color.rgb*RGB1.rgb*2.0);
	color.rgb = saturate(c1 + color2.rgb*RGB2.a*RGB2.rgb*2.0);

	// GTA III tint:
	//RGB1 *= 0.6;
	//color.rgb = color*(1.0-RGB1.a) + RGB1.rgb*RGB1.a;

	return float4(color.rgb, 1.0);
}
