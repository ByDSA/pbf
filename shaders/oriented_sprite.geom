#version 430

// Tarea por hacer: indicar el tipo de primitiva de entrada y el de salida.
layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

uniform float uSize2 = 0.15;
uniform mat4 uProjectionMatrix;

in vec4 vColor[];
out vec2 gTexCoord;
out vec4 gColor;

void main()
{
// Generar como salida el siguiente strip de triángulos (con 2 triángulos)
//     v2    v3
//     *-----*
//     |\    |
//     | \   |
//     |  \  |
//     |   \ |
//     *-----*
//    v0     v1
//
//  Donde v0 = pos_punto_entrada +  vec4(-uSize2, -uSize2, 0.0, 0.0), etc.
//  Para cada vértice de la primitiva de salida guardamos en gl_Position sus coordenadas multiplicadas por la proyección
//  También guardamos en la variables de salida sus coordenadas de textura y color

	vec4 pos = gl_in[0].gl_Position;
	vec4 v0 = pos +  vec4(-uSize2, -uSize2, 0.0, 0.0);
	vec4 v1 = pos +  vec4(uSize2, -uSize2, 0.0, 0.0);
	vec4 v2 = pos +  vec4(-uSize2, uSize2, 0.0, 0.0);
	vec4 v3 = pos +  vec4(uSize2, uSize2, 0.0, 0.0);

	gColor = vColor[0];

	gl_Position = uProjectionMatrix * v0;
	gTexCoord = vec2(0, 0);
	EmitVertex();

	gl_Position = uProjectionMatrix * v1;
	gTexCoord = vec2(1, 0);
	EmitVertex();

	gl_Position = uProjectionMatrix * v2;
	gTexCoord = vec2(0, 1);
	EmitVertex();

	gl_Position = uProjectionMatrix * v3;
	gTexCoord = vec2(1, 1);
	EmitVertex();

	EndPrimitive();
}