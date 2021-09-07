#version 330 core
out vec3 fragColour;
in Vertex{
    vec2 texCoord;
} IN;

uniform sampler2D sceneDepthTex; //PositionColourBuffer
uniform sampler2D sceneNormalTex; //NormalColourBuffer
uniform sampler2D ssdo; //SSDO pass Output
uniform sampler2D ssdoBlur; // SSDO blur pass output
uniform sampler2D texLighting; //SSDO lighting output
uniform sampler2D texIndirectLight; //SSDO indirect lighting output
uniform sampler2D texIndirectLightBlur; //SSDO indirect blur lighting output
uniform sampler2D albedo;

uniform int draw_mode;

void main()
{
    vec3 fragPos = texture(sceneDepthTex, IN.texCoord).xyz;
    vec3 normal = texture(sceneNormalTex, IN.texCoord).rgb;	
	vec3 lighting = texture(texLighting, IN.texCoord).rgb;
	float depth = texture(sceneDepthTex, IN.texCoord).a;
	vec3 directionalLight = texture(ssdo, IN.texCoord).rgb;
	vec3 directionalLightBlur = texture(ssdoBlur, IN.texCoord).rgb;
	vec3 indirectLight = texture(texIndirectLight, IN.texCoord).rgb;
	vec3 indirectLightBlur = texture(texIndirectLightBlur, IN.texCoord).rgb;
    vec3 albedoColour = texture(albedo, IN.texCoord).rgb;
	
    switch(draw_mode){
    case(1):
        fragColour = lighting;
        break;
	case(2):
        fragColour = lighting + directionalLight;
        break;
	case(3):
        fragColour = lighting + directionalLightBlur;
        break;
	case(4):
        fragColour = lighting + directionalLightBlur + indirectLight;
        break;
	case(5):
        fragColour = lighting + directionalLightBlur + indirectLightBlur;
        break;
    case(6):
        fragColour = vec3(depth / 5.0);
        break;
    case(7):
        fragColour = fragPos;
        break;
    case(8):
        fragColour = normal;
        break;
    case(9):
        fragColour = directionalLight;
        break;
	case(10):
        fragColour = directionalLightBlur;
        break;
	case(11):
        fragColour = indirectLight;
        break;
	case(12):
        fragColour = indirectLightBlur;
        break;
    case(13):
        fragColour = albedoColour;
        break;
    }
		
}