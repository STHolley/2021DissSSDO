#version 330 core
out vec3 fragColour;
in Vertex{
	vec2 texCoord;
} IN;

uniform sampler2D sceneDepthTex;
uniform sampler2D sceneNormalTex;
uniform sampler2D noiseTex;
uniform sampler2D lightingTex;

uniform int kernelSize;
uniform vec3 samples[64]; //Max 64

float radius = 5.0;

uniform vec2 windowSize;

uniform mat4 projMatrix;

void main()
{
    vec3 fragPos = texture(sceneDepthTex, IN.texCoord).xyz;
    vec3 normal = texture(sceneNormalTex, IN.texCoord).rgb;

    vec2 noiseScale = vec2(windowSize.x / float(textureSize(noiseTex, 0).x), windowSize.y / float(textureSize(noiseTex, 0).y));
    vec3 randomVec = texture(noiseTex, IN.texCoord * noiseScale).xyz;
    
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    
	vec3 indirectLight = vec3(0.0, 0.0, 0.0);

    for(int i = 0; i < kernelSize; ++i)
    {
        vec3 samp = TBN * samples[i];
        samp = fragPos + samp * radius; 
        
        vec4 offset = vec4(samp, 1.0);
        offset = projMatrix * offset;
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;
        
        //Sample details
        float sampleDepth = -texture(sceneDepthTex, offset.xy).w;
		vec3 sampleNormal = texture(sceneNormalTex, offset.xy).rgb;
		vec3 samplePos = texture(sceneDepthTex, offset.xy).xyz;
		vec3 sampleColor = texture(lightingTex, offset.xy).xyz;
         
		indirectLight += (sampleDepth >= samp.z ? 1.0 : 0.0) * max(dot(sampleNormal, normalize(fragPos - samplePos)), 0.0) * sampleColor;
    }
    indirectLight /= kernelSize;
	
	fragColour = indirectLight * 10;
}