// Skybox shader
// #type fragment
#version 430

layout(location = 0) out vec4 finalColor;
layout(location = 1) out int o_EntityID;
// layout(location = 1) out vec4 finalColor2; // output to 2nd color attachment of the render framebuffer

uniform samplerCube u_Texture;
uniform float u_TextureLod;
uniform float u_Exposure;

in vec3 v_Position;

void main()
{
	finalColor = textureLod(u_Texture, v_Position, u_TextureLod);
	finalColor.rgb *= u_Exposure; // originally used in Shaders/Hazel/SceneComposite
	
	o_EntityID = -1;
	// finalColor2 = vec4(0.0, 0.0, 0.0, 0.0); // output to 2nd color attachment of the render framebuffer
}
