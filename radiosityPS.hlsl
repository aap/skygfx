float4 main(uniform sampler2D frameSmall : register(s0),
            uniform float3 colors : register(c0),

            in float2 Tex0 : TEXCOORD0) : COLOR0
{
	float4 color = tex2D(frameSmall, Tex0);
	// a value of 2.0 for colors.z seems to reproduce the bug in pcsx2 closely
	float3 blurframe = saturate(color.rgb*colors.z - float3(1,1,1)*colors.x);
	return float4(blurframe*colors.y, 1.0);
}
