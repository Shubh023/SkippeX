#version 330 core

layout(location = 0) in vec3 position;

out vec4 fColor;


uniform mat4 transform;

void main()
{
	gl_Position = transform * vec4(position, 1.0f);

	/*
	if (position.x < 0.0) {
		fColor = vec4(1.0, 0.0, 0.0, 1.0);
	}
	else if (position.x == 0) {
		fColor = vec4(0.0, 1.0, 0.0, 1.0);
	}
	else {
		fColor = vec4(0.0, 0.0, 1.0, 1.0);
	}
	*/
}
