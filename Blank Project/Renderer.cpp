#include "Renderer.h"
#include "..\nclgl\HeightMap.h"
#include "..\nclgl\Camera.h"
#include "..\nclgl\Light.h"
#include <algorithm>
#include <random>

const int LIGHT_NUM = 64;
#define SHADOWSIZE 4096
const int POST_PASSES = 10;

Renderer::Renderer(Window& w) : OGLRenderer(w) {
	//Load meshes
	sphere = Mesh::LoadFromMeshFile("Sphere.msh");
	//cone = Mesh::LoadFromMeshFile("Cone.msh");
	//cube = Mesh::LoadFromMeshFile("Cube.msh");
	quad = Mesh::GenerateQuad();
	heightMap = new HeightMap(TEXTUREDIR"noise.png");

	kernels = new float[192];

	srand(time(NULL));
	for (int i = 0; i < 64; ++i)
	{
		Vector3 sample(((float)rand() / (float)(RAND_MAX / 2)) - 1.0, ((float)rand() / (float)(RAND_MAX / 2)) - 1.0, ((float)rand() / (float)(RAND_MAX / 2)) - 1.0);
		sample = sample.Normalised();
		ssdoKernel.push_back(sample);
		kernels[i * 3 + 0] = ssdoKernel[i].x;
		kernels[i * 3 + 1] = ssdoKernel[i].y;
		kernels[i * 3 + 2] = ssdoKernel[i].z;
	}

	//Load Textures
	glGetString(GL_EXTENSIONS);
	earthTex = SOIL_load_OGL_texture(TEXTUREDIR"Barren Reds.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	//earthBump = SOIL_load_OGL_texture(TEXTUREDIR"cw/AsphaltBump.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	overgrowthTex = SOIL_load_OGL_texture(TEXTUREDIR"cw/GrassDiffuse.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	//overgrowthBump = SOIL_load_OGL_texture(TEXTUREDIR"cw/GrassBump.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	//overgrowthMap = SOIL_load_OGL_texture(TEXTUREDIR"cw/GrassNoise.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	//waterTex = SOIL_load_OGL_texture(TEXTUREDIR"cw/WaterDiffuse.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	//waterBump = SOIL_load_OGL_texture(TEXTUREDIR"cw/WaterBump.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	
	//buildingTex = SOIL_load_OGL_texture(TEXTUREDIR"cw/BuildingDiffuse.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	//buildingBump = SOIL_load_OGL_texture(TEXTUREDIR"cw/BuildingBump.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	//buildingWindows = SOIL_load_OGL_texture(TEXTUREDIR"cw/BuildingWindow.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	noise = SOIL_load_OGL_texture(TEXTUREDIR"noise2.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	cubeMap = SOIL_load_OGL_cubemap(TEXTUREDIR"cw/skybox/miramar_rt.tga", 
		TEXTUREDIR"cw/skybox/miramar_lf.tga", 
		TEXTUREDIR"cw/skybox/miramar_up.tga", 
		TEXTUREDIR"cw/skybox/miramar_dn.tga", 
		TEXTUREDIR"cw/skybox/miramar_bk.tga", 
		TEXTUREDIR"cw/skybox/miramar_ft.tga", 
		SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);


	SetTextureRepeating(earthTex, true);
	//SetTextureRepeating(earthBump, true);
	SetTextureRepeating(overgrowthTex, true);
	//SetTextureRepeating(overgrowthMap, true);
	//SetTextureRepeating(waterTex, true);
	//SetTextureRepeating(waterBump, true);
	//SetTextureRepeating(buildingWindows, true);
	SetTextureRepeating(noise, true);

	//Vector3 heightMapSize = heightMap->GetHeightMapSize();

	camera = new Camera(0, 0, Vector3(0, 400, 0));

	//Load Shaders
	geomShader = new Shader("cw/ssdoGeomVert.glsl", "cw/ssdoGeomFrag.glsl");
	ssdoShader = new Shader("cw/ssdoVert.glsl", "cw/ssdoFrag.glsl");
	blurShader = new Shader("cw/ssdoVert.glsl", "cw/ssdoBlur.glsl");
	lightingShader = new Shader("cw/ssdoVert.glsl", "cw/ssdoLighting.glsl");
	indirectShader = new Shader("cw/ssdoVert.glsl", "cw/ssdoIndirect.glsl");
	iBlurShader = new Shader("cw/ssdoVert.glsl", "cw/ssdoBlur.glsl");
	accLightShader = new Shader("cw/ssdoVert.glsl", "cw/ssdoAccLight.glsl");
	skyboxShader = new Shader("cw/SkyboxVertex.glsl", "cw/SkyboxFragment.glsl");

	BindShader(geomShader);
	glUniform1i(glGetUniformLocation(geomShader->GetProgram(), "albedo"), 0);
	BindShader(ssdoShader);
	glUniform1i(glGetUniformLocation(ssdoShader->GetProgram(), "sceneDepthTex"), 0);
	glUniform1i(glGetUniformLocation(ssdoShader->GetProgram(), "sceneNormalTex"), 1);
	glUniform1i(glGetUniformLocation(ssdoShader->GetProgram(), "noiseTex"), 2);
	glUniform1i(glGetUniformLocation(ssdoShader->GetProgram(), "skybox"), 3);
	BindShader(blurShader);
	glUniform1i(glGetUniformLocation(blurShader->GetProgram(), "ssdoInput"), 0);
	BindShader(lightingShader);
	glUniform1i(glGetUniformLocation(lightingShader->GetProgram(), "sceneDepthTex"), 0);
	glUniform1i(glGetUniformLocation(lightingShader->GetProgram(), "sceneNormalTex"), 1);
	glUniform1i(glGetUniformLocation(lightingShader->GetProgram(), "sceneColourTex"), 2);
	BindShader(indirectShader);
	glUniform1i(glGetUniformLocation(indirectShader->GetProgram(), "sceneDepthTex"), 0);
	glUniform1i(glGetUniformLocation(indirectShader->GetProgram(), "sceneNormalTex"), 1);
	glUniform1i(glGetUniformLocation(indirectShader->GetProgram(), "noiseTex"), 2);
	glUniform1i(glGetUniformLocation(indirectShader->GetProgram(), "lightingTex"), 3);
	BindShader(iBlurShader);
	glUniform1i(glGetUniformLocation(iBlurShader->GetProgram(), "ssdoInput"), 0);
	BindShader(accLightShader);
	glUniform1i(glGetUniformLocation(accLightShader->GetProgram(), "sceneDepthTex"), 0);
	glUniform1i(glGetUniformLocation(accLightShader->GetProgram(), "sceneNormalTex"), 1);
	glUniform1i(glGetUniformLocation(accLightShader->GetProgram(), "ssdo"), 2);
	glUniform1i(glGetUniformLocation(accLightShader->GetProgram(), "ssdoBlur"), 3);
	glUniform1i(glGetUniformLocation(accLightShader->GetProgram(), "texLighting"), 4);
	glUniform1i(glGetUniformLocation(accLightShader->GetProgram(), "texIndirectLight"), 5);
	glUniform1i(glGetUniformLocation(accLightShader->GetProgram(), "texIndirectLightBlur"), 6);
	BindShader(skyboxShader);
	glUniform1i(glGetUniformLocation(skyboxShader->GetProgram(), "skybox"), 0);
	glUniform1i(glGetUniformLocation(skyboxShader->GetProgram(), "depthTex"), 1);
	glUniform1i(glGetUniformLocation(skyboxShader->GetProgram(), "texAccLight"), 2);

	//Buffers
	//geometry buffer
	glGenFramebuffers(1, &geomFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, geomFBO);
	//Texture buffers
	glGenTextures(1, &positionColourBuffer);
	glBindTexture(GL_TEXTURE_2D, positionColourBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w.GetScreenSize().x, w.GetScreenSize().y, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, positionColourBuffer, 0);
	// - Normal color buffer
	glGenTextures(1, &normalColourBuffer);
	glBindTexture(GL_TEXTURE_2D, normalColourBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w.GetScreenSize().x, w.GetScreenSize().y, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normalColourBuffer, 0);
	// - Albedo color buffer
	glGenTextures(1, &colourColourBuffer);
	glBindTexture(GL_TEXTURE_2D, colourColourBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w.GetScreenSize().x, w.GetScreenSize().y, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, colourColourBuffer, 0);
	// - Depth color buffer
	glGenTextures(1, &depthColourBuffer);
	glBindTexture(GL_TEXTURE_2D, depthColourBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w.GetScreenSize().x, w.GetScreenSize().y, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, depthColourBuffer, 0);
	// - Tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
	GLuint attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(4, attachments);
	// - Create and attach depth buffer (renderbuffer)
	glGenRenderbuffers(1, &depthRenderBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depthRenderBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w.GetScreenSize().x, w.GetScreenSize().y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Also create framebuffer to hold SSDO processing stage 
	glGenFramebuffers(1, &ssdoFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, ssdoFBO);
	glGenTextures(1, &ssdoColorBuffer);
	glBindTexture(GL_TEXTURE_2D, ssdoColorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w.GetScreenSize().x, w.GetScreenSize().y, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssdoColorBuffer, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "SSDO Framebuffer not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glGenFramebuffers(1, &ssdoBlurFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, ssdoBlurFBO);
	glGenTextures(1, &ssdoColorBufferBlur);
	glBindTexture(GL_TEXTURE_2D, ssdoColorBufferBlur);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w.GetScreenSize().x, w.GetScreenSize().y, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssdoColorBufferBlur, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "SSDO Blur Framebuffer not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	glGenFramebuffers(1, &ssdoLightingFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, ssdoLightingFBO);
	glGenTextures(1, &ssdoColorBufferLighting);
	glBindTexture(GL_TEXTURE_2D, ssdoColorBufferLighting);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w.GetScreenSize().x, w.GetScreenSize().y, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssdoColorBufferLighting, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "SSDO Lighting Framebuffer not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	glGenFramebuffers(1, &ssdoIndirectFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, ssdoIndirectFBO);
	glGenTextures(1, &ssdoColorBufferIndirect);
	glBindTexture(GL_TEXTURE_2D, ssdoColorBufferIndirect);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w.GetScreenSize().x, w.GetScreenSize().y, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssdoColorBufferIndirect, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "SSDO Indirect Framebuffer not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glGenFramebuffers(1, &ssdoIndirectBlurFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, ssdoIndirectBlurFBO);
	glGenTextures(1, &ssdoColorBufferIndirectBlur);
	glBindTexture(GL_TEXTURE_2D, ssdoColorBufferIndirectBlur);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w.GetScreenSize().x, w.GetScreenSize().y, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssdoColorBufferIndirectBlur, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "SSDO Indirect Blur Framebuffer not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glGenFramebuffers(1, &ssdoAccLightFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, ssdoAccLightFBO);
	glGenTextures(1, &ssdoColorBufferAccLight);
	glBindTexture(GL_TEXTURE_2D, ssdoColorBufferAccLight);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w.GetScreenSize().x, w.GetScreenSize().y, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssdoColorBufferAccLight, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "SSDO Accumulate Light Framebuffer not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	
	projMatrix = Matrix4::Perspective(1, 10000, (float)width / (float)height, 55);

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	currentFrame = 0;
	frameTime = 0.0f;
	init = true;

	for (int i = 0; i < 192; i += 3) {
		std::cout << abs(kernels[i]) << " " << abs(kernels[i + 1]) << " " << abs(kernels[i + 2]) << "\n";
	}
}

Renderer::~Renderer(void) {
	// Delete shaders and buffers
	//delete geomShader;

	//glDeleteTextures(1, &positionColourBuffer);
}

void Renderer::GenerateScreenTexture(GLuint& into, bool depth) {
	glGenTextures(1, &into);
	glBindTexture(GL_TEXTURE_2D, into);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	GLuint format = depth ? GL_DEPTH_COMPONENT24 : GL_RGBA8;
	GLuint type = depth ? GL_DEPTH_COMPONENT : GL_RGBA;

	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, type, GL_UNSIGNED_BYTE, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	viewMatrix = camera->BuildMatrixView();
	projMatrix = Matrix4::Perspective(1, 10000, (float)width / (float)height, 55);

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_1))
		drawMode = 1;
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_2))
		drawMode = 2;
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_3))
		drawMode = 3;
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_4))
		drawMode = 4;
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_5))
		drawMode = 5;
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_6))
		drawMode = 6;
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_7))
		drawMode = 7;
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_8))
		drawMode = 8;
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_9))
		drawMode = 9;
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_0))
		drawMode = 10;
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_P))
		drawMode = 11;
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_O))
		drawMode = 12;
}

void Renderer::RenderScene() {
	glClearColor(0.1, 0.1, 0.1, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GeomPass();
	SSDOPass();
	SSDOBlurPass();
	SSDOLightPass();
	SSDOIndirectPass();
	SSDOIndirectBlurPass();
	SSDOAccLightPass();
	SSDOSkyboxPass();
}

void Renderer::GeomPass() {
	glBindFramebuffer(GL_FRAMEBUFFER, geomFBO);
	BindShader(geomShader);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, earthTex);
	modelMatrix = Matrix4::Translation(Vector3(2000, 250, 2000)) * Matrix4::Rotation(90, Vector3(0,0,0)) * Matrix4::Scale(Vector3(1, 1, 1));
	UpdateShaderMatrices();
	heightMap->Draw();
	modelMatrix = Matrix4::Translation(Vector3(2000, 300, 2000)) * Matrix4::Scale(Vector3(100, 100, 100));
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, overgrowthTex);
	UpdateShaderMatrices();
	sphere->Draw();
	modelMatrix = Matrix4::Translation(Vector3(0, 0, 0));
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::SSDOPass() {
	glBindFramebuffer(GL_FRAMEBUFFER, ssdoFBO);
	glClear(GL_COLOR_BUFFER_BIT);
	BindShader(ssdoShader);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, positionColourBuffer);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, normalColourBuffer);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, noise);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
	glUniform1fv(glGetUniformLocation(ssdoShader->GetProgram(), "samples"), 192, kernels);
	UpdateShaderMatrices();
	quad->Draw();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::SSDOBlurPass() {
	glBindFramebuffer(GL_FRAMEBUFFER, ssdoBlurFBO);
	glClear(GL_COLOR_BUFFER_BIT);
	BindShader(blurShader);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, ssdoColorBuffer);
	UpdateShaderMatrices();
	quad->Draw();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::SSDOLightPass() {
	glBindFramebuffer(GL_FRAMEBUFFER, ssdoLightingFBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	BindShader(lightingShader);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, positionColourBuffer);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, normalColourBuffer);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, colourColourBuffer);
	// Also send light relevant uniforms
	// Update attenuation parameters
	const GLfloat linear = 0.09;
	const GLfloat quadratic = 0.032;
	Vector3 lightPos2 = camera->BuildMatrixView() * lightPos;
	glUniform1f(glGetUniformLocation(lightingShader->GetProgram(), "light.Linear"), linear);
	glUniform1f(glGetUniformLocation(lightingShader->GetProgram(), "light.Quadratic"), quadratic);
	glUniform3fv(glGetUniformLocation(lightingShader->GetProgram(), "light.Position"), 1, &lightPos2.x);
	glUniform3fv(glGetUniformLocation(lightingShader->GetProgram(), "light.Color"), 1, &lightCol.x);
	glUniform1f(glGetUniformLocation(lightingShader->GetProgram(), "light.Radius"), 500.0f);
	UpdateShaderMatrices();
	quad->Draw();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::SSDOIndirectPass() {
	glBindFramebuffer(GL_FRAMEBUFFER, ssdoIndirectFBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	BindShader(indirectShader);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, positionColourBuffer);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, normalColourBuffer);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, noise);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, ssdoColorBufferLighting);
	glUniform1fv(glGetUniformLocation(indirectShader->GetProgram(), "samples"), 192, kernels);
	UpdateShaderMatrices();
	quad->Draw();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::SSDOIndirectBlurPass() {
	glBindFramebuffer(GL_FRAMEBUFFER, ssdoIndirectBlurFBO);
	glClear(GL_COLOR_BUFFER_BIT);
	BindShader(iBlurShader);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, ssdoColorBufferIndirect);
	UpdateShaderMatrices();
	quad->Draw();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::SSDOAccLightPass() {
	glBindFramebuffer(GL_FRAMEBUFFER, ssdoAccLightFBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	BindShader(accLightShader);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, positionColourBuffer);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, normalColourBuffer);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, ssdoColorBuffer);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, ssdoColorBufferBlur);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, ssdoColorBufferLighting);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, ssdoColorBufferIndirect);
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, ssdoColorBufferIndirectBlur);
	glUniform1i(glGetUniformLocation(accLightShader->GetProgram(), "draw_mode"), drawMode);
	UpdateShaderMatrices();
	quad->Draw();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::SSDOSkyboxPass() {
	glDepthFunc(GL_ALWAYS);
	BindShader(skyboxShader);
	glUniform1i(glGetUniformLocation(skyboxShader->GetProgram(), "draw_mode"), drawMode);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthColourBuffer);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, ssdoColorBufferAccLight);
	UpdateShaderMatrices();
	quad->Draw();
	glDepthFunc(GL_LESS); // Set depth function back to default
}
