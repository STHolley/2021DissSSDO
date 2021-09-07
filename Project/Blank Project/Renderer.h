#pragma once
#include "../nclgl/OGLRenderer.h"
#include "..\nclgl\SceneNode.h"
#include "..\nclgl\MeshAnimation.h"
#include "..\nclgl\MeshMaterial.h"
class Camera;
class Mesh;

class Renderer : public OGLRenderer {
public:
	Renderer(Window& parent);
	~Renderer(void);

	void RenderScene() override;
	void UpdateScene(float dt) override;

protected:
	void GeomPass();
	void SSDOPass();
	void SSDOBlurPass();
	void SSDOLightPass();
	void SSDOIndirectPass();
	void SSDOIndirectBlurPass();
	void SSDOAccLightPass();
	void SSDOSkyboxPass();

	float lerp(float x, float y, float t) { return x + t * (y - x); };

	int drawMode = 13; //Starting draw mode in acc light shader
	Vector2 windowSize; //render window size

	//Shaders
	Shader* geomShader;
	Shader* ssdoShader;
	Shader* blurShader;
	Shader* lightingShader;
	Shader* indirectShader;
	Shader* iBlurShader;
	Shader* accLightShader;
	Shader* skyboxShader;

	//Frame Buffer Objects
	GLuint geomFBO;
	GLuint ssdoFBO;
	GLuint ssdoBlurFBO;
	GLuint ssdoLightingFBO;
	GLuint ssdoIndirectFBO;
	GLuint ssdoIndirectBlurFBO;
	GLuint ssdoAccLightFBO;

	//G-Buffers
	GLuint positionColourBuffer; //From Geom step
	GLuint normalColourBuffer; //From Geom step
	GLuint colourColourBuffer; //From Geom step
	GLuint depthColourBuffer; //From Geom step
	GLuint depthRenderBuffer; //From Geom step
	GLuint ssdoColorBuffer; //From SSDO step
	GLuint ssdoColorBufferBlur; //From first blur step
	GLuint ssdoColorBufferLighting; //From Lighting step
	GLuint ssdoColorBufferIndirect; //From Indirect Lighting step
	GLuint ssdoColorBufferIndirectBlur; //From second blur step
	GLuint ssdoColorBufferAccLight; //From buffer accumulation step

	std::vector<Vector3> ssdoKernel; //Random vectors used for SSDO

	//Scene point light information
	Vector3 lightCol = Vector3(201/510.0, 226/ 510.0, 255/ 510.0); 
	Vector3 lightPos = Vector3(-2, 2, 2);

	//Meshes
	Mesh* sphere;
	Mesh* quad;
	Mesh* actor;

	Camera* camera; //Scene camera

	//Textures
	GLuint cubeMap;
	GLuint earthTex;
	GLuint earthBump;
	GLuint redTex;
	GLuint greenTex;
	GLuint overgrowthTex;
	GLuint overgrowthBump;
	GLuint noiseTexture;
};