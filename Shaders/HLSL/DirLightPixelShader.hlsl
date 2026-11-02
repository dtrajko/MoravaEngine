Texture2D Color: register(t0);
sampler ColorSampler: register(s0);

Texture2D Normal: register(t1);
sampler NormalSampler: register(s1);

struct PS_INPUT
{
	float4 position: SV_POSITION;
	float3 normal:   NORMAL0;
	float2 texcoord: TEXCOORD0;
	float3 direction_to_camera: DIRECTIONTOCAMERA0;
	row_major float3x3 tbn: TBN0;
};

cbuffer constant: register(b0)
{
	row_major float4x4 m_model;
	row_major float4x4 m_view;
	row_major float4x4 m_proj;
	float3 m_light_direction;
	float3 m_camera_position;
	float3 m_light_position;
	float m_light_radius;
	float m_time;
};


float4 psmain(PS_INPUT input) : SV_TARGET
{
	float4 colorSample = Color.Sample(ColorSampler, float2(input.texcoord.x, 1.0 - input.texcoord.y));
	float4 normalSample = Normal.Sample(NormalSampler, float2(input.texcoord.x, 1.0 - input.texcoord.y));

	float4 normal;
	float4 color;

	if (normalSample.x == 0.0 && normalSample.y == 0.0 && normalSample.z == 0.0)
	{
		normal.xyz = input.normal;
	}
	else
	{
		normal.xyz = (normalSample.xyz * 2.0) - 1.0;
		normal.xyz = mul(normal.xyz, input.tbn);
	}

	color = colorSample;

	float dot_nl = dot(m_light_direction.xyz, input.tbn[2]);

	// Ambient light
	float ka = 20.0;
	float3 ia = float3(0.09, 0.082, 0.082);
	ia *= (color.rgb);

	float3 ambient_light = ka * ia;

	// Diffuse light
	float kd = 0.7;
	float amount_diffuse_light = dot(m_light_direction.xyz, normal.xyz);

	float3 id = float3(1.0, 1.0, 1.0);
	id *= (color.rgb);

	float3 diffuse_light = kd * id * amount_diffuse_light;

	// Specular light
	float ks = 0.8;
	float is = float3(1.0, 1.0, 1.0);
	float3 reflected_light = reflect(m_light_direction.xyz, normal.xyz);
	float shininess = 40.0;
	float amount_specular_light = 0;
	if (dot_nl > 0) {
		amount_specular_light = pow(max(0.0, dot(reflected_light, input.direction_to_camera)), shininess);
	}
	float3 specular_light = ks * amount_specular_light * is;

	// Final light
	float3 final_light = ambient_light + diffuse_light + specular_light;

	return float4(final_light, 1.0) * color;
}
