#version 330 core
in vec3 position;
in vec3 normal;
in vec2 texCoord;

out Vertex {
    vec2 texCoord;
    vec3 normal;
    vec3 worldPos;
} OUT;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

void main()
{
    vec4 viewPos = viewMatrix * modelMatrix * vec4(position, 1.0f);
    OUT.worldPos = viewPos.xyz * 0.5; 
    gl_Position = projMatrix * viewPos;
    
    OUT.texCoord = texCoord;
    
    mat3 normalMatrix = transpose(inverse(mat3(viewMatrix * modelMatrix)));
    OUT.normal = normalMatrix * normal;
}