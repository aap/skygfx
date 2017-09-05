float4 main(uniform sampler2D Diffuse : register(s0),
            uniform float4 RGB1 : register(c0),
            uniform float4 RGB2 : register(c1),

            in float2 Tex0 : TEXCOORD0) : COLOR0
{
	// GTA VC trails
	float a = 30/255.0f;
	float4 doublec = saturate(RGB1*2);
	float4 dst = tex2D(Diffuse, Tex0);
	float4 prev = dst;
	for(int i = 0; i < 5; i++){
		float4 tmp = dst*(1-a) + prev*doublec*a;
		tmp += prev*RGB1;
		tmp += prev*RGB1;
		prev = saturate(tmp);
	}
	return prev;
}
