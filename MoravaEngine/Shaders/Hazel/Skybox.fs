// Skybox shader
// #type fragment
#version 430

layout(location = 0) out vec4 finalColor;
layout(location = 1) out int o_EntityID;
// layout(location = 1) out vec4 finalColor2; // output to 2nd color attachment of the render framebuffer

uniform samplerCube u_Texture;
uniform float u_TextureLod;
uniform float u_Exposure;
uniform float u_Gamma;

in vec3 v_Position;

void main()
{
	finalColor = textureLod(u_Texture, v_Position, u_TextureLod);
	finalColor.rgb *= u_Exposure; // originally used in Shaders/Hazel/SceneComposite
	// Optional tonemapping + gamma correction (linear HDR -> display) for renderers without a composite pass; 0.0 (default) = off
	if (u_Gamma > 0.0)
	{
		vec3 x = max(finalColor.rgb, vec3(0.0)) * 0.6; // 0.6 = pre-exposure of the ACES fit
		x = clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0); // ACES filmic (Narkowicz fit)
		finalColor.rgb = pow(x, vec3(1.0 / u_Gamma));
	}
	
	o_EntityID = -1;
	// finalColor2 = vec4(0.0, 0.0, 0.0, 0.0); // output to 2nd color attachment of the render framebuffer
}
