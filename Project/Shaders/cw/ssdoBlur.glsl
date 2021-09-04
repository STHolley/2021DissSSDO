#version 330 core
in Vertex{
    vec2 texCoord;
} IN;
out vec3 fragColor;

uniform sampler2D ssdoInput;

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(ssdoInput, 0));
    vec3 result = vec3(0.0, 0.0, 0.0);
    for (int x = -2; x < 2; ++x) 
    {
        for (int y = -2; y < 2; ++y) 
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(ssdoInput, IN.texCoord + offset).rgb;
        }
    }
    fragColor = result / (4.0 * 4.0);
}  