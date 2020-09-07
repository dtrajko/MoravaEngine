// type fragment
#version 430

in vec2 v_TexCoord;

uniform sampler2D u_Texture;

layout(location=0) out vec4 outColor;

uniform float u_Exposure;

void main()
{
    const float gamma     = 2.2;
    const float pureWhite = 1.0;

    vec3 color = texture(u_Texture, v_TexCoord).rgb * u_Exposure;
    // Reinhard tonemapping operator.
    // see: "Photographic Tone Reproduction for Digital Images", eq. 4
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float mappedLuminance = (luminance * (1.0 + luminance / (pureWhite * pureWhite))) / (1.0 + luminance);

    // Scale color by ratio of average luminances.
    vec3 mappedColor = (mappedLuminance / luminance) * color;

    // Gamma correction.
    outColor = vec4(pow(mappedColor, vec3(1.0/gamma)), 1.0);
}
