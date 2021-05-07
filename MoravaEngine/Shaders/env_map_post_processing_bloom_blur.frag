#version 430

layout(location = 0) out vec4 o_Color;

in vec2 v_TexCoord;

uniform sampler2D u_Texture;
uniform bool u_Horizontal;

void main()
{
#if 0
    // From learnopengl.com
	float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

	vec2 tex_offset = 1.0 / textureSize(u_Texture, 0); // gets size of single texel
    vec3 result = texture(u_Texture, v_TexCoord).rgb * weight[0]; // current fragment's contribution
    if (u_Horizontal)
    {
	    for(int i = 1; i < 5; ++i)
        {
            result += texture(u_Texture, v_TexCoord + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
            result += texture(u_Texture, v_TexCoord - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
        }
    }
    else
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(u_Texture, v_TexCoord + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
            result += texture(u_Texture, v_TexCoord - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
        }
    }
	o_Color = vec4(result, 1.0);
#else
    // From https://www.shadertoy.com/view/Xltfzj

	float Pi = 6.28318530718; // Pi*2
    
    // GAUSSIAN BLUR SETTINGS {{{
    float Directions =32.0; // BLUR DIRECTIONS (Default 16.0 - More is better but slower)
    float Quality = 6.0; // BLUR QUALITY (Default 4.0 - More is better but slower)
    float Size = 16.0; // BLUR SIZE (Radius)
    // GAUSSIAN BLUR SETTINGS }}}
   
    vec2 Radius = Size/textureSize(u_Texture, 0);

    vec3 result = texture(u_Texture, v_TexCoord).rgb;
    vec2 uv = v_TexCoord;
    // Blur calculations
    for( float d=0.0; d<Pi; d+=Pi/Directions)
    {
		for(float i=1.0/Quality; i<=1.0; i+=1.0/Quality)
        {
			result += texture( u_Texture, uv+vec2(cos(d),sin(d))*Radius*i).rgb;
        }
    }
    
    // Output to screen
    result /= Quality * Directions - 15.0;
    o_Color = vec4(result, 1.0);
#endif

}
