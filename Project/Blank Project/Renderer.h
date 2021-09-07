#pragma once
#include "../nclgl/OGLRenderer.h"
#include "..\nclgl\SceneNode.h"
#include "..\nclgl\Frustum.h";
#include "..\nclgl\MeshAnimation.h"
#include "..\nclgl\MeshMaterial.h"
class Camera;
class Mesh;
class HeightMap;

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
	vector<Vector3> lightCycle;
	int stationNum = 0;

	int drawMode = 1;
	Frustum frameFrustum;

	Shader* geomShader;
	Shader* ssdoShader;
	Shader* blurShader;
	Shader* lightingShader;
	Shader* indirectShader;
	Shader* iBlurShader;
	Shader* accLightShader;
	Shader* skyboxShader;

	GLuint skyboxVAO, skyboxVBO;

	GLuint geomFBO;
	GLuint positionColourBuffer;
	GLuint normalColourBuffer;
	GLuint colourColourBuffer;
	GLuint depthColourBuffer;
	GLuint depthRenderBuffer;

	GLuint ssdoFBO, ssdoBlurFBO, ssdoLightingFBO, ssdoIndirectFBO, ssdoIndirectBlurFBO, ssdoAccLightFBO;
	GLuint ssdoColorBuffer, ssdoColorBufferBlur, ssdoColorBufferLighting, ssdoColorBufferIndirect, ssdoColorBufferIndirectBlur, ssdoColorBufferAccLight;

	std::vector<Vector3> ssdoKernel;

	Vector3 lightCol = Vector3(201/510.0, 226/ 510.0, 255/ 510.0);
	Vector3 lightPos = Vector3(-2, 2, 2);

	HeightMap* heightMap;

	Mesh* sphere;
	Mesh* quad;
	Mesh* actor;

	Camera* camera;
	Vector3 pointTo;

	GLuint cubeMap;

	GLuint earthTex;
	GLuint earthBump;
	GLuint redTex;
	GLuint greenTex;
	GLuint overgrowthTex;
	GLuint overgrowthBump;
	
	GLuint noiseTexture;

	GLuint noise;
};