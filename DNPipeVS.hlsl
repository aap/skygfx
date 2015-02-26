struct VS_INPUT
{
	float4 Position		: POSITION;
	float3 Normal		: NORMAL;
	float2 TexCoord		: TEXCOORD0;
	float4 NightColor	: COLOR0;
	float4 DayColor		: COLOR1;
};

struct VS_OUTPUT {
	float4 position		: POSITION;
	float3 texcoord0	: TEXCOORD0;
	float3 texcoord1	: TEXCOORD1;
	float3 texcoord2	: TEXCOORD2;	// unused here
	float4 color		: COLOR0;
	float4 envcolor		: COLOR1;
	float4 speccolor	: TEXCOORD3;	// unused here
};

float4x4	world : register(c0);
float4x4	view : register(c4);
float4x4	proj : register(c8);
float4		materialColor : register(c12);
float3		surfaceProps : register(c13);
float3		ambientLight : register(c14);
float3		shaderVars : register(c15);	// colorScale, balance, reflSwitch
float4		reflData : register(c16);	// shininess, intensity
float4		envXform : register(c17);

VS_OUTPUT main(in VS_INPUT In)
{
	VS_OUTPUT Out;

	Out.position = mul(proj, mul(view, mul(world, In.Position)));
	Out.texcoord0 = float3(0.0, 0.0, 0.0);
	Out.texcoord1 = float3(0.0, 0.0, 0.0);
	Out.texcoord2 = float3(0.0, 0.0, 0.0);
	Out.envcolor = float4(0.0, 0.0, 0.0, 0.0);
	Out.speccolor = float4(0.0, 0.0, 0.0, 0.0);

	Out.color.rgb = (In.DayColor*(1.0-shaderVars[1]) + In.NightColor*shaderVars[1]).rgb;
	Out.color.a = In.NightColor.a;

	Out.texcoord0.xy = In.TexCoord;
	if(shaderVars[2] == 1.0){		// PS2 style
		float4 camNormal;
		camNormal.xyz = normalize(mul((float3x3)view, mul((float3x3)world, In.Normal.xyz)));
		camNormal.w = 0.0;
		Out.texcoord1.xy = camNormal.xy;// - envXform.xy;
		Out.texcoord1.xy *= -envXform.zw;
		Out.envcolor = float4(192.0, 192.0, 192.0, 0.0)/128.0*reflData.x*reflData.y;
	}else if(shaderVars[2] == 2.0){		// fixed PC style
		Out.texcoord1.xy = mul((float3x3)view, mul((float3x3)world, In.Normal)).xy;
		Out.envcolor = float4(1.0, 1.0, 1.0, 0.0)*reflData.x;
	}else if(shaderVars[2] == 3.0){		// bugged PC style
		Out.texcoord1.xy = mul((float3x3)view, mul((float3x3)world, In.Normal)).xy;
		Out.envcolor = Out.color;
	}

	Out.color *= materialColor / shaderVars[0];
	Out.color.rgb += ambientLight*surfaceProps.x;
	Out.color = saturate(Out.color)*shaderVars[0];

	return Out;
}
