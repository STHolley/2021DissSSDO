#version 330 core
out vec3 FragColor;
in Vertex{
    vec2 texCoord;
} IN;

uniform sampler2D sceneDepthTex;
uniform sampler2D sceneNormalTex;
uniform sampler2D ssdo;
uniform sampler2D ssdoBlur;
uniform sampler2D texLighting;
uniform sampler2D texIndirectLight;
uniform sampler2D texIndirectLightBlur;

uniform mat4 viewMatrix;

uniform int draw_mode;

void main()
{
    // Get input for SSDO algorithm
    vec3 fragPos = texture(sceneDepthTex, IN.texCoord).xyz;
    vec3 normal = texture(sceneNormalTex, IN.texCoord).rgb;	
	vec3 lighting = texture(texLighting, IN.texCoord).rgb;
	float depth = texture(sceneDepthTex, IN.texCoord).a;
	vec3 directionalLight = texture(ssdo, IN.texCoord).rgb;
	vec3 directionalLightBlur = texture(ssdoBlur, IN.texCoord).rgb;
	vec3 indirectLight = texture(texIndirectLight, IN.texCoord).rgb;
	vec3 indirectLightBlur = texture(texIndirectLightBlur, IN.texCoord).rgb;
	
	// Based on which of the 1 - 9, 0, P, O keys we pressed, show specific buffer values
    if(draw_mode == 1)
        FragColor = lighting;
	else if(draw_mode == 2)
        FragColor = lighting + directionalLight;
	else if(draw_mode == 3)
        FragColor = lighting + directionalLightBlur;
	else if(draw_mode == 4)
        FragColor = lighting + directionalLightBlur + indirectLight;
	else if(draw_mode == 5)
        FragColor = lighting + directionalLightBlur + indirectLightBlur;
    else if(draw_mode == 6)
        FragColor = vec3(depth / 50.0);
    else if(draw_mode == 7)
        FragColor = fragPos;
    else if(draw_mode == 8)
        FragColor = normal;
    else if(draw_mode == 9)
        FragColor = directionalLight;
	else if(draw_mode == 10)
        FragColor = directionalLightBlur;
	else if(draw_mode == 11)
        FragColor = indirectLight;
	else if(draw_mode == 12)
        FragColor = indirectLightBlur;
		
}