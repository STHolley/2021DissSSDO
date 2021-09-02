#version 330 core
out vec3 FragColor;
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
    
    // Then calculate lighting as usual
    vec3 viewDir  = normalize(-FragPos); // Viewpos is (0.0.0)
    // Diffuse
    vec3 lightDir = normalize(light.Position - FragPos);
    vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * light.Color;
    // Specular
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(Normal, halfwayDir), 0.0), 50.0);
    vec3 specular = light.Color * spec;
    // Attenuation
    float distance = length(light.Position - FragPos);
    float attenuation = 1.0 / (1.0 + light.Linear * distance + light.Quadratic * distance * distance);
    diffuse *= attenuation;
    specular *= attenuation;

	FragColor = diffuse + specular;
}
