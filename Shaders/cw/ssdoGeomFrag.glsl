#version 330 core
layout (location = 0) out vec4 sceneDepthTex;
layout (location = 1) out vec3 sceneNormalTex;
layout (location = 2) out vec3 colourTex;
layout (location = 3) out float depthTex;

uniform sampler2D albedo;
uniform sampler2D normal;

in Vertex{
	vec4 colour;
	vec2 texCoord;
	vec3 normal;
	vec3 tangent;
	vec3 binormal;
	vec3 worldPos;
} IN;

const float NEAR = 0.1;
const float FAR = 50.0f;
float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // Back to NDC 
    return (2.0 * NEAR * FAR) / (FAR + NEAR - z * (FAR - NEAR));	
}

void main()
{    
    sceneDepthTex = vec4(IN.worldPos, LinearizeDepth(gl_FragCoord.z));
	depthTex = sceneDepthTex.a;
    sceneNormalTex = normalize(IN.normal);

	mat3 TBN = mat3(normalize(IN.tangent), normalize(IN.binormal), normalize(IN.normal));

	vec4 diffuse = texture(albedo, IN.texCoord);
	vec3 bumpNormal = texture(normal, IN.texCoord).rgb;
	//sceneNormalTex = (sceneNormalTex * 0.75) + (bumpNormal * 0.25);


    vec3 bumpTex = texture(normal, IN.texCoord).rgb * 2.0 - 1.0;
    colourTex = texture(albedo, IN.texCoord).rgb;
}