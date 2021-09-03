#version 330 core
out vec3 fragColour;
in Vertex{
	vec2 texCoord;
} IN;

uniform sampler2D sceneDepthTex;
uniform sampler2D sceneNormalTex;
uniform sampler2D sceneColourTex;

struct Light {
    vec3 Position;
    vec3 Color;
    float Linear;
    float Quadratic;
    float Radius;
};
uniform Light light;


void main()
{
    // Retrieve data from gbuffer
    vec3 FragPos = texture(sceneDepthTex, IN.texCoord).rgb;
    vec3 Normal = texture(sceneNormalTex, IN.texCoord).rgb;
    vec3 Diffuse = texture(sceneColourTex, IN.texCoord).rgb;

    vec3 viewDir  = normalize(FragPos); // viewpos is (0.0.0) in view-space
    // Then calculate lighting as usual
    // Diffuse
    vec3 lightDir = normalize(light.Position - FragPos);
    vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * light.Color;
    // Specular
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(Normal, halfwayDir), 0.0), 50.0);
    vec3 specular = light.Color * spec;
    // Attenuation
    float dist = length(light.Position - FragPos);
    float attenuation = 1.0 / (1.0 + light.Linear * dist + light.Quadratic * dist * dist);
    diffuse *= attenuation;
    specular *= attenuation;

	fragColour = diffuse + specular;
}
