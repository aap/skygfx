float4 main(uniform sampler2D tex : register(s0),

            in float2 Tex0 : TEXCOORD0) : COLOR0
{
	float4 color = tex2D(tex, Tex0);
	return color*float4(14, 14, 14, 255)/255.0;
}
