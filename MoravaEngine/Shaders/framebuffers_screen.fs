#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;

uniform int effect;

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

void DefaultColors()
{
    // Normal colors
    FragColor = texture(screenTexture, TexCoords);
}

void InvertColors()
{
    // Post-processing - invert colors
    FragColor = vec4(vec3(1.0 - texture(screenTexture, TexCoords)), 1.0);
}

void Grayscale()
{
    // Post-processing - grayscale
    FragColor = texture(screenTexture, TexCoords);
    float average = 0.2126 * FragColor.r + 0.7152 * FragColor.g + 0.0722 * FragColor.b;
    FragColor = vec4(average, average, average, 1.0); // shades of gray
}

void Nightvision()
{
    // Post-processing - nightvision
    FragColor = texture(screenTexture, TexCoords);
    float average = 0.2126 * FragColor.r + 0.7152 * FragColor.g + 0.0722 * FragColor.b;
    FragColor = vec4(0.0, average, 0.0, 1.0); // shades of green (nightvision)
}

void Kernel(float kernel[9])
{
    vec3 sampleTex[9];
    for(int i = 0; i < 9; i++)
    {
        sampleTex[i] = vec3(texture(screenTexture, TexCoords.st + offsets[i]));
    }
    vec3 col = vec3(0.0);
    for(int i = 0; i < 9; i++)
        col += sampleTex[i] * kernel[i];
    
    FragColor = vec4(col, 1.0);
}

void main()
{
    if (effect == 0)
        DefaultColors();
    if (effect == 1)
        InvertColors();
    if (effect == 2)
        Grayscale();
    if (effect == 3)
        Nightvision();
    if (effect == 4)
        Kernel(kernel_sharpen);
    if (effect == 5)
        Kernel(kernel_blur);
}
