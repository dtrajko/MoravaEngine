#version 330

in vec2 aPosition;

out vec2 blurTextureCoords[11];
out vec2 vPosition;

uniform float targetHeight;

void main(void){

	gl_Position = vec4(aPosition, 0.0, 1.0);
	vec2 centerTexCoords = aPosition * 0.5 + 0.5;
	float pixelSize = 1.0 / targetHeight;

	for (int i = -5; i <= 5; i++) {
		blurTextureCoords[i+5] = centerTexCoords + vec2(0.0, pixelSize * i);
	}

	vPosition = aPosition;
}
