#version 330 core
in vec3 position;
in vec2 texCoord;

out Vertex{
	vec2 texCoord;
} OUT;

void main()
{
    gl_Position = vec4(position, 1.0f);
    OUT.texCoord = texCoord;
}