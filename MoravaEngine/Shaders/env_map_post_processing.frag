#version 450 core

in vec2 vTexCoord;

uniform sampler2D u_AlbedoMap;

out vec4 FragColor;


// kernel calculation
const float offset = 1.0 / 300.0;

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

// kernel sharpen
float kernel_sharpen[9] = float[]
(
    -1, -1, -1,
    -1,  9, -1,
    -1, -1, -1
);

// blur    
float kernel_blur[9] = float[]
(
    1.0 / 16, 2.0 / 16, 1.0 / 16,
    2.0 / 16, 4.0 / 16, 2.0 / 16,
    1.0 / 16, 2.0 / 16, 1.0 / 16  
);

void Kernel(float kernel[9])
{
    vec3 sampleTex[9];
    for(int i = 0; i < 9; i++)
    {
        sampleTex[i] = vec3(texture(u_AlbedoMap, vTexCoord.st + offsets[i]));
    }
    vec3 col = vec3(0.0);
    for(int i = 0; i < 9; i++)
        col += sampleTex[i] * kernel[i];
    
    FragColor = vec4(col, 1.0);
}

void main()
{
	// FragColor = vec4(vec3(texture(u_AlbedoMap, vTexCoord) + 0.5), 1.0);
    Kernel(kernel_blur);
}
