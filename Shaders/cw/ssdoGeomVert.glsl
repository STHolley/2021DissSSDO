#version 330 core
in vec3 position;
in vec3 normal;
in vec2 texCoord;
in vec4 tangent;

out Vertex{
	vec4 colour;
	vec2 texCoord;
	vec3 normal;
	vec3 tangent;
	vec3 binormal;
	vec3 worldPos;
} OUT;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

void main()
{
    vec4 viewPos = viewMatrix * modelMatrix * vec4(position, 1.0f);
    OUT.worldPos = viewPos.xyz / 500.0f; 
    gl_Position = projMatrix * viewPos;
    
    OUT.texCoord = texCoord;
    
    mat3 normalMatrix = transpose(inverse(mat3(viewMatrix * modelMatrix)));

    vec3 wNormal = normalize(normalMatrix * normalize(normal));
	vec3 wTangent = normalize(normalMatrix * normalize(tangent.xyz));

	OUT.normal = wNormal;
	OUT.tangent = wTangent;
	OUT.binormal = cross(wTangent, wNormal) * tangent.w;
}