#version 430

// Tarea por hacer: recoger (como datos de entrada) los datos de salida del geometry shader.
in vec4 gColor;
in vec2 gTexCoord;

uniform sampler2D uSpriteTex;

out vec4 fFragColor;

void main()
{
	//fFragColor = gColor;	

// Tarea por hacer: una vez activo el geometry shader, sustuir el anterior código por el siguiente:
	if ( length(gTexCoord - 0.5) > 0.38 ) discard;
	vec4 color = texture(uSpriteTex, gTexCoord);
	fFragColor = color * gColor;
}