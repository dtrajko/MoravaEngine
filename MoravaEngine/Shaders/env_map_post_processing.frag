#version 450 core

out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D u_AlbedoMap;

uniform int u_Effect;

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
    FragColor = texture(u_AlbedoMap, vTexCoord);
}

void InvertColors()
{
    // Post-processing - invert colors
    FragColor = vec4(vec3(1.0 - texture(u_AlbedoMap, vTexCoord)), 1.0);
}

void Grayscale()
{
    // Post-processing - grayscale
    FragColor = texture(u_AlbedoMap, vTexCoord);
    float average = 0.2126 * FragColor.r + 0.7152 * FragColor.g + 0.0722 * FragColor.b;
    FragColor = vec4(average, average, average, 1.0); // shades of gray
}

void Nightvision()
{
    // Post-processing - nightvision
    FragColor = texture(u_AlbedoMap, vTexCoord);
    float average = 0.2126 * FragColor.r + 0.7152 * FragColor.g + 0.0722 * FragColor.b;
    FragColor = vec4(0.0, average, 0.0, 1.0); // shades of green (nightvision)
}

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

void ShadesOfGray()
{
    vec4 TextureColor = texture(u_AlbedoMap, vTexCoord);

    int levels = 4;
    float gray = round((TextureColor.x + TextureColor.y + TextureColor.z) * levels) / (3 * levels);
    FragColor = vec4(gray, gray, gray, 1.0);
}

void Colors8Bit()
{
    vec4 TextureColor = texture(u_AlbedoMap, vTexCoord);

    float r = 0.0;
    float g = 0.0;
    float b = 0.0;

    r = round(TextureColor.x * 8) / 8;
    g = round(TextureColor.y * 8) / 8;
    b = round(TextureColor.z * 4) / 4;

    FragColor = vec4(r, g, b, 1.0);
}

void GaussianBlur()
{
	float Pi = 6.28318530718; // Pi*2
    
    // GAUSSIAN BLUR SETTINGS {{{
    float Directions = 16.0; // BLUR DIRECTIONS (Default 16.0 - More is better but slower)
    float Quality = 4.0; // BLUR QUALITY (Default 4.0 - More is better but slower)
    float Size = 16.0; // BLUR SIZE (Radius)
    // GAUSSIAN BLUR SETTINGS }}}

    vec2 Radius = Size / textureSize(u_AlbedoMap, 0);

    vec3 result = texture(u_AlbedoMap, vTexCoord).rgb;
    vec2 uv = vTexCoord;

    // Blur calculations
    for(float d = 0.0; d < Pi; d += Pi / Directions)
    {
		for(float i = 1.0 / Quality; i<=1.0; i += 1.0 / Quality)
        {
			result += texture(u_AlbedoMap, uv + vec2(cos(d), sin(d)) * Radius * i).rgb;
        }
    }
    
    // Output to screen
    result /= Quality * Directions - 15.0;
    FragColor = vec4(result, 1.0);
}

void main()
{
    if (u_Effect == 0) {
        DefaultColors();
	} else if (u_Effect == 1) {
        InvertColors();
	} else if (u_Effect == 2) {
        Grayscale();
    } else if (u_Effect == 3) {
        Nightvision();
    } else if (u_Effect == 4) {
        Kernel(kernel_sharpen);
    } else if (u_Effect == 5) { 
        Kernel(kernel_blur);
    } else if (u_Effect == 6) { 
        ShadesOfGray();
    } else if (u_Effect == 7) { 
        Colors8Bit();
    } else if (u_Effect == 8) { 
        GaussianBlur();
	} else {
		DefaultColors();
	}
}
