float4x4	world : register(c0);
float4x4	view : register(c4);
float4x4	proj : register(c8);

float4		shaderVars : register(c15);	// colorScale, nightMult, dayMult, reflSwitch
float4		reflData : register(c16);	// shininess, intensity
float4		envXform : register(c17);
float4		basecolor : register(c24);

sampler2D tex1 : register(s1);

struct VS_INPUT {
	float4 Position		: POSITION;
	float3 Normal		: NORMAL;
	float2 TexCoord		: TEXCOORD0;
	float4 NightColor	: COLOR0;
	float4 DayColor		: COLOR1;
};

struct VS_OUTPUT {
	float4 position		: POSITION;
	float3 texcoord1	: TEXCOORD0;
	float4 envcolor		: COLOR0;
};

VS_OUTPUT
main(VS_INPUT IN)
{	
	VS_OUTPUT OUT;
	OUT.position = mul(proj, mul(view, mul(world, IN.Position)));
	OUT.texcoord1 = float3(0.0, 0.0, 0.0);
	OUT.envcolor = float4(0.0, 0.0, 0.0, 0.0);

	if(shaderVars[3] == 1.0){		// PS2 style
		float4 camNormal;
		camNormal.xyz = normalize(mul((float3x3)view, mul((float3x3)world, IN.Normal.xyz)));
		camNormal.w = 0.0;
		OUT.texcoord1.xy = camNormal.xy;// - envXform.xy;
		OUT.texcoord1.xy *= -envXform.zw;
		OUT.envcolor = float4(192.0, 192.0, 192.0, 0.0)/128.0*reflData.x*reflData.y;
	}else if(shaderVars[3] == 2.0){		// fixed PC style
		OUT.texcoord1.xy = mul((float3x3)view, mul((float3x3)world, IN.Normal)).xy;
		OUT.envcolor = float4(1.0, 1.0, 1.0, 0.0)*reflData.x;
	}else if(shaderVars[3] == 3.0){		// bugged PC style
		OUT.texcoord1.xy = mul((float3x3)view, mul((float3x3)world, IN.Normal)).xy;
		// repeat the calculation from the first pass to get the color again...
		float4 color = IN.DayColor*shaderVars[2] + IN.NightColor*shaderVars[1];
		color.a = IN.NightColor.a;
		color += basecolor;
		OUT.envcolor = color;
	}
	OUT.texcoord1.z = 1.0;
	return OUT;
}
