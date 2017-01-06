float4 main(uniform sampler2D tex : register(s0),
            uniform float4 color : register(c0),

            in float2 Tex0 : TEXCOORD0) : COLOR0
{
	float4 c = tex2D(tex, Tex0);
	return c*color*255.0/128.0;
}
