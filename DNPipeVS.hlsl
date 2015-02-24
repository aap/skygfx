struct VS_INPUT
{
	float4 Position		: POSITION;
	float3 Normal		: NORMAL;
	float2 TexCoord		: TEXCOORD0;
	float4 NightColor	: COLOR0;
	float4 DayColor		: COLOR1;
};

struct VS_OUTPUT
{
	float4 Position		: POSITION;
	float2 TexCoord0	: TEXCOORD0;
	float2 TexCoord1	: TEXCOORD1;
	float4 Color		: COLOR0;
};

float4x4	world : register(c0);
float4x4	view : register(c4);
float4x4	proj : register(c8);
float4		materialColor : register(c12);
float3		surfaceProps : register(c13);
float3		ambientLight : register(c14);
float3		shaderVars;	// colorScale, balance, reflSwitch

VS_OUTPUT main(in VS_INPUT In)
{
	VS_OUTPUT Out;

	Out.Position = mul(proj, mul(view, mul(world, In.Position)));
	Out.TexCoord0 = In.TexCoord;
	Out.TexCoord1 = float2(0.0, 0.0);
	if(shaderVars[2] != 0.0){
		Out.TexCoord1 = mul(view, mul(world, In.Normal)).xy;
	}

	float4 color;
	color.rgb = (In.DayColor*(1.0-shaderVars[1]) + In.NightColor*shaderVars[1]).rgb;
	color.a = In.NightColor.a;
	Out.Color = color * materialColor / shaderVars[0];
	Out.Color.rgb += ambientLight*surfaceProps[0];
	Out.Color = saturate(Out.Color)*shaderVars[0];

	return Out;
}
