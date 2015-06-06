#version 430  

in vec4 aPosition;
in vec4 aColor;

uniform mat4 uModelViewProjMatrix;
uniform mat4 uModelViewMatrix;

out vec4 vColor;

void main() {
	vColor = aColor;

	gl_Position = uModelViewMatrix * vec4(aPosition.xyz, 1.0);
}
