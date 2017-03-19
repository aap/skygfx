float4 main(uniform sampler2D Diffuse : register(s0),
            uniform float4 RGB1 : register(c0),
            uniform float4 RGB2 : register(c1),

            in float2 Tex0 : TEXCOORD0,
            in float2 Tex1 : TEXCOORD1) : COLOR0
{
	// GTA III real trails:
	float4 dst = tex2D(Diffuse, Tex0);
	float4 prev = dst;
	for(int i = 0; i < 5; i++){
		float4 tmp = dst*(1.0-RGB1.a) + prev*RGB1*RGB1.a;
		prev = tmp;
	}
	return float4(prev.rgb, 1.0);
}
