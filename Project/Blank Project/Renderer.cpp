#include "Renderer.h"
#include "..\nclgl\HeightMap.h"
#include "..\nclgl\Camera.h"
#include "..\nclgl\Light.h"
#include <algorithm>
#include <random>

const int KERNEL_SIZE = 32;
#define SHADOWSIZE 4096
const int POST_PASSES = 10;

Renderer::Renderer(Window& w) : OGLRenderer(w) {
	//Load meshes
	sphere = Mesh::LoadFromMeshFile("Sphere.msh");
	//cone = Mesh::LoadFromMeshFile("Cone.msh");
	//cube = Mesh::LoadFromMeshFile("Cube.msh");
	quad = Mesh::GenerateQuad();
	heightMap = new HeightMap(TEXTUREDIR"noise.png");
	actor = Mesh::LoadFromMeshFile("Role_T.msh");

	//Load Textures
	glGetString(GL_EXTENSIONS);
	earthTex = SOIL_load_OGL_texture(TEXTUREDIR"Barren Reds.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	earthBump = SOIL_load_OGL_texture(TEXTUREDIR"cw/AsphaltBump.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	overgrowthTex = SOIL_load_OGL_texture(TEXTUREDIR"cw/GrassDiffuse.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	overgrowthBump = SOIL_load_OGL_texture(TEXTUREDIR"cw/GrassBump.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	
	redTex = SOIL_load_OGL_texture(TEXTUREDIR"red.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	greenTex = SOIL_load_OGL_texture(TEXTUREDIR"green.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	cubeMap = SOIL_load_OGL_cubemap(TEXTUREDIR"cw/skybox/miramar_rt.tga",
		TEXTUREDIR"cw/skybox/miramar_lf.tga",
		TEXTUREDIR"cw/skybox/miramar_up.tga",
		TEXTUREDIR"cw/skybox/miramar_dn.tga",
		TEXTUREDIR"cw/skybox/miramar_bk.tga",
		TEXTUREDIR"cw/skybox/miramar_ft.tga",
		SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);


	SetTextureRepeating(earthTex, true);
	SetTextureRepeating(earthBump, true);
	SetTextureRepeating(overgrowthTex, true);
	SetTextureRepeating(overgrowthBump, true);
	SetTextureRepeating(redTex, true);
	SetTextureRepeating(greenTex, true);
	

	Vector3 heightMapSize = heightMap->GetHeightMapSize();

	camera = new Camera(0, 0, Vector3(0.5, 4, 0.5));

	//Load Shaders
	geomShader = new Shader("cw/ssdoGeomVert.glsl", "cw/ssdoGeomFrag.glsl");
	ssdoShader = new Shader("cw/ssdoVert.glsl", "cw/ssdoFrag.glsl");
	blurShader = new Shader("cw/ssdoVert.glsl", "cw/ssdoBlur.glsl");
	lightingShader = new Shader("cw/ssdoVert.glsl", "cw/ssdoLighting.glsl");
	indirectShader = new Shader("cw/ssdoVert.glsl", "cw/ssdoIndirect.glsl");
	iBlurShader = new Shader("cw/ssdoVert.glsl", "cw/ssdoBlur.glsl");
	accLightShader = new Shader("cw/ssdoVert.glsl", "cw/ssdoAccLight.glsl");
	skyboxShader = new Shader("cw/SkyboxVertex.glsl", "cw/SkyboxFragment.glsl");

	std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between [0.0, 1.0]
	std::default_random_engine generator;
	std::cout << "samples:\n";
	for (int i = 0; i < KERNEL_SIZE; ++i)
	{
		Vector3 sample(
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator)
		);
		sample = sample.Normalised();
		sample = sample * randomFloats(generator);
		float scale = (float)i / 64.0;
		scale = lerp(0.1f, 1.0f, scale * scale);
		sample = sample * scale;
		ssdoKernel.push_back(sample);
		std::cout << sample;
	}

	std::cout << "\n";
	vector<Vector3> ssaoNoise;
	for (unsigned int i = 0; i < 16; i++)
	{
		std::cout << "\nNext Tex Pixel\nx:";
		Vector3 temp(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f);
		ssaoNoise.push_back(temp);
	}
	
	glGenTextures(1, &noiseTexture);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0].x);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	BindShader(geomShader);
	glUniform1i(glGetUniformLocation(geomShader->GetProgram(), "albedo"), 0);
	glUniform1i(glGetUniformLocation(geomShader->GetProgram(), "normal"), 1);
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
	glUniform1i(glGetUniformLocation(accLightShader->GetProgram(), "albedo"), 7);
	BindShader(skyboxShader);
	glUniform1i(glGetUniformLocation(skyboxShader->GetProgram(), "skybox"), 0);
	glUniform1i(glGetUniformLocation(skyboxShader->GetProgram(), "depthTex"), 1);
	glUniform1i(glGetUniformLocation(skyboxShader->GetProgram(), "texAccLight"), 2);

	//Buffers
	glGenFramebuffers(1, &geomFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, geomFBO);
	glGenTextures(1, &positionColourBuffer);
	glBindTexture(GL_TEXTURE_2D, positionColourBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w.GetScreenSize().x, w.GetScreenSize().y, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, positionColourBuffer, 0);
	glGenTextures(1, &normalColourBuffer);
	glBindTexture(GL_TEXTURE_2D, normalColourBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w.GetScreenSize().x, w.GetScreenSize().y, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normalColourBuffer, 0);
	glGenTextures(1, &colourColourBuffer);
	glBindTexture(GL_TEXTURE_2D, colourColourBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w.GetScreenSize().x, w.GetScreenSize().y, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, colourColourBuffer, 0);
	glGenTextures(1, &depthColourBuffer);
	glBindTexture(GL_TEXTURE_2D, depthColourBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w.GetScreenSize().x, w.GetScreenSize().y, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, depthColourBuffer, 0);

	GLuint attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(4, attachments);
	glGenRenderbuffers(1, &depthRenderBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depthRenderBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w.GetScreenSize().x, w.GetScreenSize().y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

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
	glDepthFunc(GL_LESS);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	init = true;
}

Renderer::~Renderer(void) {
	delete geomShader;
	delete ssdoShader;
	delete blurShader;
	delete lightingShader;
	delete indirectShader;
	delete iBlurShader;
	delete accLightShader;
	delete skyboxShader;

	glDeleteTextures(1, &positionColourBuffer);
	glDeleteTextures(1, &normalColourBuffer);
	glDeleteTextures(1, &colourColourBuffer);
	glDeleteTextures(1, &depthColourBuffer);
	glDeleteTextures(1, &depthRenderBuffer);
	glDeleteTextures(1, &ssdoColorBuffer);
	glDeleteTextures(1, &ssdoColorBufferBlur);
	glDeleteTextures(1, &ssdoColorBufferLighting);
	glDeleteTextures(1, &ssdoColorBufferIndirect);
	glDeleteTextures(1, &ssdoColorBufferIndirectBlur);
	glDeleteTextures(1, &ssdoColorBufferAccLight);

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
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_L))
		drawMode = 13;
}

void Renderer::RenderScene() {
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
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
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, earthBump);
	modelMatrix = Matrix4::Translation(Vector3(0, 2.5, 0)) * Matrix4::Rotation(90, Vector3(1,0,0)) * Matrix4::Scale(Vector3(10, 10, 10));
	UpdateShaderMatrices();
	quad->Draw();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, redTex);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, NULL);
	modelMatrix = Matrix4::Translation(Vector3(0, 12.5, 10)) * Matrix4::Rotation(0, Vector3(1, 0, 0)) * Matrix4::Scale(Vector3(10, 10, 10));
	UpdateShaderMatrices();
	quad->Draw();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, greenTex);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, NULL);
	modelMatrix = Matrix4::Translation(Vector3(10, 12.5, 0)) * Matrix4::Rotation(90, Vector3(0, 1, 0)) * Matrix4::Scale(Vector3(10, 10, 10));
	UpdateShaderMatrices();
	quad->Draw();
	modelMatrix = Matrix4::Translation(Vector3(0, 3, 0)) * Matrix4::Scale(Vector3(10, 10, 10));
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, overgrowthTex);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, overgrowthBump);
	UpdateShaderMatrices();
	//sphere->Draw();
	modelMatrix = Matrix4::Translation(Vector3(7.5, 2.5, 7.5)) * Matrix4::Rotation(225, Vector3(0, 1, 0)) * Matrix4::Scale(Vector3(2, 2, 2));
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, overgrowthTex);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, overgrowthBump);
	UpdateShaderMatrices();
	actor->Draw();
	modelMatrix.ToIdentity();
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
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
	for (GLuint i = 0; i < KERNEL_SIZE; ++i)
		glUniform3fv(glGetUniformLocation(ssdoShader->GetProgram(), ("samples[" + std::to_string(i) + "]").c_str()), 1, &ssdoKernel[i].x);
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
	const GLfloat linear = 0.09;
	const GLfloat quadratic = 0.032;
	Matrix4 camMat = camera->BuildMatrixView();
	camMat.SetPositionVector(Vector3(0, 0, 0));
	Vector3 test = camMat * (lightPos);// +((camera->GetPosition() - Vector3(5, 40, 5)) / 50);
	std::cout << test;
	glUniform1f(glGetUniformLocation(lightingShader->GetProgram(), "light.Linear"), linear);
	glUniform1f(glGetUniformLocation(lightingShader->GetProgram(), "light.Quadratic"), quadratic);
	glUniform3fv(glGetUniformLocation(lightingShader->GetProgram(), "light.Position"), 1, &test.x);
	glUniform3fv(glGetUniformLocation(lightingShader->GetProgram(), "light.Colour"), 1, &lightCol.x);
	glUniform1f(glGetUniformLocation(lightingShader->GetProgram(), "light.Radius"), 5);
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
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, ssdoColorBufferLighting);
	for (GLuint i = 0; i < KERNEL_SIZE; ++i)
		glUniform3fv(glGetUniformLocation(indirectShader->GetProgram(), ("samples[" + std::to_string(i) + "]").c_str()), 1, &ssdoKernel[i].x);
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
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, colourColourBuffer);
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
	glDepthFunc(GL_LESS);
}
