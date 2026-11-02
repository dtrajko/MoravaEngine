#version 330 core

out vec4 FragColor;

struct Material
{
    sampler2D diffuse;
    sampler2D specular;
    sampler2D normalMap;
    float shininess;
}; 

struct Light
{
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
 
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in mat3 TBN;

uniform vec3 viewPos;
uniform bool enableNormalMap;
uniform Material material;
uniform Light light;

vec3 GetNormal()
{
    vec3 normal;

    if (!enableNormalMap)
    {
	    normal = normalize(Normal);
	    return normal;
    }

	normal = texture(material.normalMap, TexCoords).rgb;
	normal = normal * 2.0 - 1.0;
	normal = normalize(TBN * normal);
	return normal;
}

void main()
{
    // ambient
    vec4 ambient = vec4(light.ambient, 1.0) * texture(material.diffuse, TexCoords).rgba;

    // diffuse
    vec3 norm = GetNormal(); // normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec4 diffuse = vec4(light.diffuse, 1.0) * diff * texture(material.diffuse, TexCoords).rgba;

    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec4 specular = vec4(light.specular, 1.0) * spec * texture(material.specular, TexCoords).rgba;

    // spotlight (soft edges)
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = (light.cutOff - light.outerCutOff);
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    diffuse  *= intensity;
    specular *= intensity;

    // attenuation
    float distance    = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

    vec4 result = ambient + diffuse + specular;
    // FragColor = vec4(result); // transparency enabled
    FragColor = vec4(result.rgb, 1.0); // transparency disabled
}
