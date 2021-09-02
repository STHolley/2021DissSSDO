#version 330 core
layout (location = 0) out vec4 sceneDepthTex;
layout (location = 1) out vec3 sceneNormalTex;
layout (location = 2) out vec3 colourTex;
layout (location = 3) out float depthTex;

uniform sampler2D albedo;

in Vertex {
    vec2 texCoord;
    vec3 normal;
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
    colourTex = texture(albedo, IN.texCoord).rgb;
}