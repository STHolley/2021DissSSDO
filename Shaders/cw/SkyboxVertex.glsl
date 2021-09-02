#version 330 core
in vec3 position;
out Vertex {
    vec3 texCoord;
} OUT;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;


void main()
{
    vec3 pos = position;
	mat4 invproj = inverse(projMatrix);
	pos.xy *= vec2(invproj[0][0], invproj[1][1]);
	pos.z = -1.0f;

	OUT.texCoord = transpose(mat3(viewMatrix)) * normalize(pos);
	gl_Position = vec4(position, 1.0);

	//vec4 pos = projMatrix * viewMatrix * vec4(position, 1.0);
    //gl_Position = pos.xyww;
    //OUT.texCoord = position;
}  