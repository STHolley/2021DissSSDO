#version 330 core
in Vertex{
	vec2 texCoord;
} IN;

out vec3 fragColour;

uniform sampler2D sceneDepthTex;
uniform sampler2D sceneNormalTex;
uniform sampler2D noiseTex;
uniform samplerCube skybox;

uniform vec3 samples[64];

// parameters (you'd probably want to use them as uniforms to more easily tweak the effect)
int kernelSize = 64;
float radius = 50.0;

// tile noise texture over screen based on screen dimensions divided by noise size
const vec2 noiseScale = vec2(1500.0f/4.0f, 1000.0f/4.0f); 

uniform mat4 viewMatrix;
uniform mat4 projMatrix;

void main()
{
    // Get input for SSDO algorithm
    vec3 fragPos = texture(sceneDepthTex, IN.texCoord).xyz;
    vec3 normal = texture(sceneNormalTex, IN.texCoord).rgb;
    vec3 randomVec = texture(noiseTex, IN.texCoord * noiseScale).xyz;
    // Create TBN change-of-basis matrix: from tangent-space to view-space
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    // Iterate over the sample kernel and calculate occlusion factor	
    vec3 directionalLight = vec3(0.0, 0.0, 0.0);
	
    for(int i = 0; i < kernelSize; ++i)
    {
        // get sample position
        vec3 samp = TBN * samples[i]; // From tangent to view-space
        samp = fragPos + samp * radius; 
        
        // project sample position (to sample texture) (to get position on screen/texture)
        vec4 offset = vec4(samp, 1.0);
        offset = projMatrix * offset; // from view to clip-space
        offset.xyz /= offset.w; // perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0
        
        // get sample depth
        float sampleDepth = -texture(sceneDepthTex, offset.xy).w; // Get depth value of kernel sample
		vec3 sampleNormal = texture(sceneNormalTex, offset.xy).rgb;
		vec3 samplePos = texture(sceneDepthTex, offset.xy).xyz;
		
		vec4 skyboxDirection = inverse(viewMatrix) * vec4(samp - fragPos, 1.0);
		skyboxDirection /= skyboxDirection.w;
		vec3 skyboxColor = texture(skybox, skyboxDirection.xyz).xyz;
        
        // range check & accumulate
		if (sampleDepth < samp.z) {
			directionalLight += skyboxColor * dot(normal, normalize(samp - fragPos));
		}
        
    }
    directionalLight /= kernelSize * 2;
    
    fragColour = directionalLight;
}