#define limit (colors.x)
#define intensity (colors.y)

float4 main(uniform sampler2D frameSmall : register(s0),
            uniform float3 colors : register(c0),

            in float2 Tex0 : TEXCOORD0) : COLOR0
{
	float4 color = tex2D(frameSmall, Tex0);
	float3 blurframe = saturate(color.rgb*2.0 - float3(1,1,1)*limit);
	return float4(blurframe*intensity, 1.0);
}
