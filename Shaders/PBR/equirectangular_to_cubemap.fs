#version 330 core
out vec4 FragColor;
in vec3 WorldPos;

uniform sampler2D equirectangularMap;

uniform float blurLevel;
uniform float textureSize;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

// -- BEGIN Kernel Blur effect
// kernel calculation
float offset = 1.0 / textureSize / 4.0;

// blur    
float kernel_blur[9] = float[]
(
    1.0 / 16, 2.0 / 16, 1.0 / 16,
    2.0 / 16, 4.0 / 16, 2.0 / 16,
    1.0 / 16, 2.0 / 16, 1.0 / 16  
);

vec4 Kernel(float kernel[9], sampler2D inputTexture, vec2 texCoords, float offset)
{
    vec2 offsets[9] = vec2[]
    (
        vec2(-offset,  offset), // top-left
        vec2( 0.0f,    offset), // top-center
        vec2( offset,  offset), // top-right
        vec2(-offset,  0.0f),   // center-left
        vec2( 0.0f,    0.0f),   // center-center
        vec2( offset,  0.0f),   // center-right
        vec2(-offset, -offset), // bottom-left
        vec2( 0.0f,   -offset), // bottom-center
        vec2( offset, -offset)  // bottom-right    
    );

    vec3 sampleTex[9];
    for(int i = 0; i < 9; i++)
    {
        sampleTex[i] = vec3(texture(inputTexture, texCoords + offsets[i]));
    }
    vec3 col = vec3(0.0);
    for(int i = 0; i < 9; i++)
        col += sampleTex[i] * kernel[i];

    return vec4(col, 1.0);
}
// -- END Kernel Blur effect

void main()
{		
    vec2 uv = SampleSphericalMap(normalize(WorldPos));

    vec3 color;

    if (blurLevel > 0.0) {
        color = Kernel(kernel_blur, equirectangularMap, uv, offset).rgb;
    } else {
        color = texture(equirectangularMap, uv).rgb;
    }

    FragColor = vec4(color, 1.0);
}
