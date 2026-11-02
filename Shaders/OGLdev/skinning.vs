#version 330                                                                        
                                                                                    
layout (location = 0) in vec3  aPosition;                                             
layout (location = 1) in vec2  aTexCoord;                                             
layout (location = 2) in vec3  aNormal;                                               
layout (location = 3) in ivec4 aBoneIDs;
layout (location = 4) in vec4  aWeights;

out vec2 vTexCoord0;
out vec3 vNormal0;                                                                   
out vec3 vWorldPos0;                                                                 

const int MAX_BONES = 100;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec4 clipPlane;
uniform mat4 gBones[MAX_BONES];

void main()
{       
    mat4 BoneTransform = gBones[aBoneIDs[0]] * aWeights[0];
    BoneTransform     += gBones[aBoneIDs[1]] * aWeights[1];
    BoneTransform     += gBones[aBoneIDs[2]] * aWeights[2];
    BoneTransform     += gBones[aBoneIDs[3]] * aWeights[3];

    vec4 PosL    = BoneTransform * vec4(aPosition, 1.0);
    gl_Position  = projection * view * model * PosL;
    vTexCoord0    = aTexCoord;
	
	vec4 WorldPosition = model * vec4(aPosition, 1.0);
	gl_ClipDistance[0] = dot(WorldPosition, clipPlane);
	
    vec4 NormalL = BoneTransform * vec4(aNormal, 0.0);
    vNormal0      = (model * NormalL).xyz;
    vWorldPos0    = (model * PosL).xyz;                                
}
