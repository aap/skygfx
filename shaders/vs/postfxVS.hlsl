void main(in float4 position : POSITION,
	  in float4 color : COLOR0,
          in float2 texcoords0 : TEXCOORD0,
          in float2 texcoords1 : TEXCOORD1,

          out float4 position_out : POSITION,
	  out float4 color_out : COLOR0,
          out float2 texcoords0_out : TEXCOORD0,
          out float2 texcoords1_out : TEXCOORD1)
{
	position_out = position;
	color_out = color;
	texcoords0_out = texcoords0;
	texcoords1_out = texcoords1;
}
