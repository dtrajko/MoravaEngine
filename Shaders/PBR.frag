#version 330 core

out vec4 FragColor;

in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

// material parameters
uniform vec3  albedo;
uniform float metallic;
uniform float roughness;
uniform float ao;

uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;

// lights
uniform vec3 lightPositions[4];
uniform vec3 lightColors[4];

uniform vec3 camPos;
uniform float ambientIntensity;

const float PI = 3.14159265359;

vec3 GetNormalFromMap()
{
    vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(WorldPos);
    vec3 Q2  = dFdy(WorldPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);

    vec3 N   = normalize(Normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return a2 / max(denom, 0.0000001); // prevent divide by zero
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 baseReflectivity)
{
    // baseReflectivity in range 0 to 1
    // returns range of baseReflectivity to 1
    // increases as HdotV decreases (more reflectivity when surface viewed at larger angles)
    return baseReflectivity + (1.0 - baseReflectivity) * pow(1.0 - cosTheta, 5.0);
}

void main()
{
	vec3 txAlbedo     = pow(texture(albedoMap, TexCoords).rgb, vec3(2.2));
    vec3 Normal     = GetNormalFromMap();
    float txMetallic  = texture(metallicMap, TexCoords).r;
    float txRoughness = texture(roughnessMap, TexCoords).r;
    float txAO        = texture(aoMap, TexCoords).r;

    vec3  finalAlbedo = albedo * txAlbedo;
    float finalMetallic = (metallic + txMetallic) / 2.0;
    float finalRoughness = (roughness + txRoughness) / 2.0;
	float finalAO = (ao + txAO + ambientIntensity) / 3.0;

    vec3 N = normalize(Normal);
    vec3 V = normalize(camPos - WorldPos);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use baseReflectivity
    // of 0.04 and if it's a metal, use the albedo color as baseReflectivity (metallic workflow)
    vec3 baseReflectivity = vec3(0.04);
    baseReflectivity = mix(baseReflectivity, finalAlbedo, finalMetallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < 4; ++i)
    {
        // calculate per-light radiance
        vec3 L = normalize(lightPositions[i] - WorldPos);
        vec3 H = normalize(V + L);
        float distance    = length(lightPositions[i] - WorldPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance     = lightColors[i] * attenuation;

        // cook-torrance BRDF
        float NDF = DistributionGGX(N, H, finalRoughness);
        float G   = GeometrySmith(N, V, L, finalRoughness);
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), baseReflectivity);

        vec3 kS = F;

        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;

		// multiply kD by the inverse metalness such that only non-metals
        // have diffuse lighting, or a linear blend if partly metal
        // (pure metals have no diffuse light).
        kD *= 1.0 - finalMetallic;	  

        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
        vec3 specular     = numerator / max(denominator, 0.001);

        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);

        // note that 1) angle of light to surface affects specular, not just diffuse
        //           2) we mix albedo with diffuse, but not specular
        Lo += (kD * finalAlbedo / PI + specular) * radiance * NdotL;
    }

    // ambient lighting (note that the next IBL tutorial will replace
    // this ambient lighting with environment lighting)
    vec3 ambient = vec3(ambientIntensity) * finalAlbedo * finalAO;

    vec3 color = ambient + Lo;

	// HDR tonemapping
    color = color / (color + vec3(1.0));
	
	// gamma correct
    color = pow(color, vec3(1.0/2.2));
   
    FragColor = vec4(color, 1.0);
}
