#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D displayBuffer;

void main()
{

	FragColor = vec4(texture(displayBuffer, TexCoords).rrr, 1.0) * vec4(0.88, 0.97, 0.82, 1.0);
}