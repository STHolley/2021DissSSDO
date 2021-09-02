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
	void DrawShadowScene();
	void FillBuffers();
	void DrawPointLights();
	void CombineBuffers();
	void GenerateScreenTexture(GLuint& into, bool depth = false);
	void DrawGround();
	void DrawWater();
	void DrawSkybox();
	void GeomPass();
	void SSDOPass();
	void SSDOBlurPass();
	void SSDOLightPass();
	void SSDOIndirectPass();
	void SSDOIndirectBlurPass();
	void SSDOAccLightPass();
	void SSDOSkyboxPass();
	void DrawActors();
	void DrawPostProcess();
	void BuildNodeLists(SceneNode* from);
	void SortNodeLists();
	void ClearNodeLists();
	void DrawNodes(Shader* shader = NULL);
	void DrawNode(SceneNode* n, Shader* shader = NULL);

	float lerp(float x, float y, float t) { return x + t * (y - x); };
	vector<Vector3> lightCycle;
	int stationNum = 0;
	float elapsed = 0;
	bool cycling = true;

	int drawMode = 1;

	SceneNode* actors;
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
	float* kernels;

	Vector3 lightCol = Vector3(0, 1, 0);
	Vector3 lightPos = Vector3(1, -2, -1);

	HeightMap* heightMap;
	Light* pointLights;

	Mesh* sphere;
	Mesh* cone;
	Mesh* quad;
	Mesh* cube;

	Camera* camera;
	Vector3 pointTo;
	
	Mesh* actor;
	float actorCount = 5;
	float buildingCount = 10;

	MeshAnimation* anim;
	MeshMaterial* material;
	vector<GLuint> matTextures;
	Matrix4 skeletonMat;

	GLuint cubeMap;

	GLuint earthTex;
	GLuint earthBump;
	GLuint overgrowthTex;
	GLuint overgrowthBump;
	GLuint overgrowthMap;
	GLuint buildingTex;
	GLuint buildingBump;
	GLuint buildingWindows;
	

	GLuint noise;


	GLuint waterTex;
	GLuint waterBump;
	float waterRotate;
	float waterCycle;
	int currentFrame;
	float frameTime;

	bool blur = false;

	vector<SceneNode*> nodeList;
};