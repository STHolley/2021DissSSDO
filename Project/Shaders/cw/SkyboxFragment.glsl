#version 330 core

uniform samplerCube cubeTex;
uniform sampler2D depthTex;
uniform sampler2D texAccLight;
uniform int draw_mode;

in Vertex{
	vec3 texCoord;
} IN;

out vec4 fragColour;

void main()
{   
	float depth = texture(depthTex, gl_FragCoord.xy / vec2(1500,1000)).r;
	vec3 accLight = texture(texAccLight, gl_FragCoord.xy / vec2(1500, 1000)).rgb;
	if (depth < 0.099 && draw_mode <= 5)
		fragColour = texture(cubeTex, IN.texCoord);
	else
		fragColour = vec4(accLight, 1.0);
}
