#version 460 core

layout(location = 0) out vec4 color;

in vec4 fColor;

void main()
{
	// color = vec4(0.50, 0.25, 0.75, 1.0);
	color = fColor;
}
