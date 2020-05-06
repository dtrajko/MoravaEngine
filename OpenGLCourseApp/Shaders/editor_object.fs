#version 330 core

out vec4 FragColor;

uniform vec4 tintColor;
uniform bool isSelected;

void main()
{
	float illumination = 1.2;
	vec4 NewColor = tintColor;
	if (isSelected)
	{
		NewColor.x = min(NewColor.x * illumination, 1.0);
		NewColor.y = min(NewColor.y * illumination, 1.0);
		NewColor.z = min(NewColor.z * illumination, 1.0);
	}
	FragColor = NewColor;
}
