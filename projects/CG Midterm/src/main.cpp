//Just a simple handler for simple initialization stuffs
#include "BackendHandler.h"

#include <filesystem>
#include <json.hpp>
#include <fstream>

#include <Texture2D.h>
#include <Texture2DData.h>
#include <MeshBuilder.h>
#include <MeshFactory.h>
#include <NotObjLoader.h>
#include <ObjLoader.h>
#include <VertexTypes.h>
#include <ShaderMaterial.h>
#include <RendererComponent.h>
#include <TextureCubeMap.h>
#include <TextureCubeMapData.h>

#include <Timing.h>
#include <GameObjectTag.h>
#include <InputHelpers.h>

#include <IBehaviour.h>
#include <CameraControlBehaviour.h>
#include <FollowPathBehaviour.h>
#include <SimpleMoveBehaviour.h>

int main() {
	int frameIx = 0;
	float fpsBuffer[128];
	float minFps, maxFps, avgFps;
	int selectedVao = 0; // select cube by default
	int lightNum = 5;
	bool isTextured = true;
	std::vector<GameObject> controllables;

	////For Light Lerping////
	GLfloat lightTime = 0.0f;

	float t = 0.0f;
	float totalTime;

	float speed = 4.0f;

	glm::vec3 point1 = glm::vec3(-3.0f, 0.0f, 20.0f);
	glm::vec3 point2 = glm::vec3(3.0f, 0.0f, 20.0f);

	glm::vec3 checker2Point1 = glm::vec3(4.0f, 4.0f, 0.0f);
	glm::vec3 checker2Point2 = glm::vec3(8.0f, 0.0f, 0.0f);

	glm::vec3 checker3Point1 = glm::vec3(-4.0f, -4.0f, 0.0f);
	glm::vec3 checker3Point2 = glm::vec3(0.0f, 0.0f, 0.0f);

	glm::vec3 currentPos = glm::vec3(3.0f, -3.0f, 15.0f);
	glm::vec3 checker2CurrentPos = glm::vec3(4.0f, 4.0f, 0.0f);
	glm::vec3 checker3CurrentPos = glm::vec3(-4.0f, -4.0f, 0.0f);

	bool forwards = true;

	float distance = glm::distance(point2, point1);

	totalTime = distance / speed;
	////////////////////////////

	BackendHandler::InitAll();

	// Let OpenGL know that we want debug output, and route it to our handler function
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(BackendHandler::GlDebugMessage, nullptr);

	// Enable texturing
	glEnable(GL_TEXTURE_2D);

	// Push another scope so most memory should be freed *before* we exit the app
	{
#pragma region Shader and ImGui
		Shader::sptr passthroughShader = Shader::Create();
		passthroughShader->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
		passthroughShader->LoadShaderPartFromFile("shaders/passthrough_frag.glsl", GL_FRAGMENT_SHADER);
		passthroughShader->Link();

		// Load our shaders
		Shader::sptr shader = Shader::Create();
		shader->LoadShaderPartFromFile("shaders/vertex_shader.glsl", GL_VERTEX_SHADER);
		//shader->LoadShaderPartFromFile("shaders/frag_shader.glsl", GL_FRAGMENT_SHADER);
		shader->LoadShaderPartFromFile("shaders/frag_blinn_phong_textured.glsl", GL_FRAGMENT_SHADER);
		shader->Link();

		glm::vec3 lightPos = glm::vec3(0.0f, 0.0f, 5.0f);
		glm::vec3 lightDir = glm::vec3(0.0f, 0.0f, -1.0f);
		glm::vec3 lightCol = glm::vec3(1.0f, 1.0f, 1.0f);
		float     lightAmbientPow = 0.15f;
		float     lightSpecularPow = 1.0f;
		glm::vec3 ambientCol = glm::vec3(1.0f);
		float     ambientPow = 0.1f;
		float     lightLinearFalloff = 0.09f;
		float     lightQuadraticFalloff = 0.032f;

		// These are our application / scene level uniforms that don't necessarily update
		// every frame
		shader->SetUniform("u_LightPos", lightPos);
		shader->SetUniform("u_LightDir", lightDir);
		shader->SetUniform("u_LightCol", lightCol);
		shader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
		shader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
		shader->SetUniform("u_AmbientCol", ambientCol);
		shader->SetUniform("u_AmbientStrength", ambientPow);
		shader->SetUniform("u_LightAttenuationConstant", 1.0f);
		shader->SetUniform("u_LightAttenuationLinear", lightLinearFalloff);
		shader->SetUniform("u_LightAttenuationQuadratic", lightQuadraticFalloff);

		PostEffect* basicEffect;
		
		int activeEffect = 3;
		std::vector<PostEffect*> effects;

		SepiaEffect* sepiaEffect;
		GreyscaleEffect* greyscaleEffect;
		ColorCorrectEffect* colorCorrectEffect;
		BloomEffect* bloomEffect;


		// We'll add some ImGui controls to control our shader
		BackendHandler::imGuiCallbacks.push_back([&]() {
			if (ImGui::CollapsingHeader("Effect controls"))
			{
				ImGui::SliderInt("Chosen Effect", &activeEffect, 0, effects.size() - 1);

				if (activeEffect == 0)
				{
					ImGui::Text("Active Effect: Sepia Effect");

					SepiaEffect* temp = (SepiaEffect*)effects[activeEffect];
					float intensity = temp->GetIntensity();

					if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 1.0f))
					{
						temp->SetIntensity(intensity);
					}
				}
				if (activeEffect == 1)
				{
					ImGui::Text("Active Effect: Greyscale Effect");

					GreyscaleEffect* temp = (GreyscaleEffect*)effects[activeEffect];
					float intensity = temp->GetIntensity();

					if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 1.0f))
					{
						temp->SetIntensity(intensity);
					}
				}
				if (activeEffect == 2)
				{
					ImGui::Text("Active Effect: Color Correct Effect");

					ColorCorrectEffect* temp = (ColorCorrectEffect*)effects[activeEffect];
					static char input[BUFSIZ];
					ImGui::InputText("Lut File to Use", input, BUFSIZ);

					if (ImGui::Button("SetLUT", ImVec2(200.0f, 40.0f)))
					{
						temp->SetLUT(LUT3D(std::string(input)));
					}
				}
				if (activeEffect == 3)
				{
					ImGui::Text("Active Effect: Bloom Effect");

					BloomEffect* temp = (BloomEffect*)effects[activeEffect];
					float threshold = temp->GetThreshold();

					if (ImGui::SliderFloat("Threshold", &threshold, 0.0f, 1.0f))
					{
						temp->SetThreshold(threshold);
					}

					float downscale = temp->GetDownscale();

					if (ImGui::SliderFloat("Blur", &downscale, 1.0f, 5.0f))
					{
						temp->SetDownscale(downscale);
					}
				}
			}
			if (ImGui::Checkbox("Textures", &isTextured));

			if (ImGui::CollapsingHeader("Scene Level Lighting Settings"))
			{
				if (ImGui::ColorPicker3("Ambient Color", glm::value_ptr(ambientCol))) {
					shader->SetUniform("u_AmbientCol", ambientCol);
				}
				if (ImGui::SliderFloat("Fixed Ambient Power", &ambientPow, 0.01f, 1.0f)) {
					shader->SetUniform("u_AmbientStrength", ambientPow);
				}
			}
			if (ImGui::CollapsingHeader("Light Level Lighting Settings"))
			{
				if (ImGui::DragFloat3("Light Pos", glm::value_ptr(lightPos), 0.01f, -10.0f, 10.0f)) {
					shader->SetUniform("u_LightPos", lightPos);
				}
				if (ImGui::ColorPicker3("Light Col", glm::value_ptr(lightCol))) {
					shader->SetUniform("u_LightCol", lightCol);
				}
				if (ImGui::SliderFloat("Light Ambient Power", &lightAmbientPow, 0.0f, 1.0f)) {
					shader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
				}
				if (ImGui::SliderFloat("Light Specular Power", &lightSpecularPow, 0.0f, 1.0f)) {
					shader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
				}
				if (ImGui::DragFloat("Light Linear Falloff", &lightLinearFalloff, 0.01f, 0.0f, 1.0f)) {
					shader->SetUniform("u_LightAttenuationLinear", lightLinearFalloff);
				}
				if (ImGui::DragFloat("Light Quadratic Falloff", &lightQuadraticFalloff, 0.01f, 0.0f, 1.0f)) {
					shader->SetUniform("u_LightAttenuationQuadratic", lightQuadraticFalloff);
				}
			}

			auto name = controllables[selectedVao].get<GameObjectTag>().Name;
			ImGui::Text(name.c_str());
			auto behaviour = BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao]);
			ImGui::Checkbox("Relative Rotation", &behaviour->Relative);

			ImGui::Text("Q/E -> Yaw\nLeft/Right -> Roll\nUp/Down -> Pitch\nY -> Toggle Mode");

			minFps = FLT_MAX;
			maxFps = 0;
			avgFps = 0;
			for (int ix = 0; ix < 128; ix++) {
				if (fpsBuffer[ix] < minFps) { minFps = fpsBuffer[ix]; }
				if (fpsBuffer[ix] > maxFps) { maxFps = fpsBuffer[ix]; }
				avgFps += fpsBuffer[ix];
			}
			ImGui::PlotLines("FPS", fpsBuffer, 128);
			ImGui::Text("MIN: %f MAX: %f AVG: %f", minFps, maxFps, avgFps / 128.0f);
			});

#pragma endregion 

		// GL states
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glDepthFunc(GL_LEQUAL); // New 

#pragma region TEXTURE LOADING

// Load some textures from files
		Texture2D::sptr stone = Texture2D::LoadFromFile("images/Stone_001_Diffuse.png");
		Texture2D::sptr stoneSpec = Texture2D::LoadFromFile("images/Stone_001_Specular.png");
		Texture2D::sptr grass = Texture2D::LoadFromFile("images/grass.jpg");
		Texture2D::sptr noSpec = Texture2D::LoadFromFile("images/grassSpec.png");
		Texture2D::sptr box = Texture2D::LoadFromFile("images/box.bmp");
		Texture2D::sptr boxSpec = Texture2D::LoadFromFile("images/box-reflections.bmp");
		Texture2D::sptr simpleFlora = Texture2D::LoadFromFile("images/SimpleFlora.png");
		Texture2D::sptr checkerboard = Texture2D::LoadFromFile("images/Checkerboard.png");
		Texture2D::sptr whiteChecker = Texture2D::LoadFromFile("images/WhiteChecker.jpg");
		Texture2D::sptr blackChecker = Texture2D::LoadFromFile("images/BlackChecker.jpg");
		LUT3D testCube("cubes/BrightenedCorrection.cube");

		// Load the cube map
		//TextureCubeMap::sptr environmentMap = TextureCubeMap::LoadFromImages("images/cubemaps/skybox/sample.jpg");
		TextureCubeMap::sptr environmentMap = TextureCubeMap::LoadFromImages("images/cubemaps/skybox/ToonSky.jpg");

		// Creating an empty texture
		Texture2DDescription desc = Texture2DDescription();
		desc.Width = 1;
		desc.Height = 1;
		desc.Format = InternalFormat::RGB8;
		Texture2D::sptr texture2 = Texture2D::Create(desc);
		// Clear it with a white colour
		texture2->Clear();

#pragma endregion

		///////////////////////////////////// Scene Generation //////////////////////////////////////////////////
#pragma region Scene Generation

// We need to tell our scene system what extra component types we want to support
		GameScene::RegisterComponentType<RendererComponent>();
		GameScene::RegisterComponentType<BehaviourBinding>();
		GameScene::RegisterComponentType<Camera>();

		// Create a scene, and set it to be the active scene in the application
		GameScene::sptr scene = GameScene::Create("test");
		Application::Instance().ActiveScene = scene;

		// We can create a group ahead of time to make iterating on the group faster
		entt::basic_group<entt::entity, entt::exclude_t<>, entt::get_t<Transform>, RendererComponent> renderGroup =
			scene->Registry().group<RendererComponent>(entt::get_t<Transform>());

		// Create a material and set some properties for it
		ShaderMaterial::sptr stoneMat = ShaderMaterial::Create();
		stoneMat->Shader = shader;
		stoneMat->Set("s_Diffuse", stone);
		stoneMat->Set("s_Specular", stoneSpec);
		stoneMat->Set("u_Shininess", 2.0f);
		stoneMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr grassMat = ShaderMaterial::Create();
		grassMat->Shader = shader;
		grassMat->Set("s_Diffuse", grass);
		grassMat->Set("s_Specular", noSpec);
		grassMat->Set("u_Shininess", 2.0f);
		grassMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr boxMat = ShaderMaterial::Create();
		boxMat->Shader = shader;
		boxMat->Set("s_Diffuse", box);
		boxMat->Set("s_Specular", boxSpec);
		boxMat->Set("u_Shininess", 8.0f);
		boxMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr simpleFloraMat = ShaderMaterial::Create();
		simpleFloraMat->Shader = shader;
		simpleFloraMat->Set("s_Diffuse", simpleFlora);
		simpleFloraMat->Set("s_Specular", noSpec);
		simpleFloraMat->Set("u_Shininess", 8.0f);
		simpleFloraMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr checkerMat = ShaderMaterial::Create();
		checkerMat->Shader = shader;
		checkerMat->Set("s_Diffuse", checkerboard);
		checkerMat->Set("s_Specular", noSpec);
		checkerMat->Set("u_Shininess", 8.0f);
		checkerMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr whiteMat = ShaderMaterial::Create();
		whiteMat->Shader = shader;
		whiteMat->Set("s_Diffuse", whiteChecker);
		whiteMat->Set("s_Specular", noSpec);
		whiteMat->Set("u_Shininess", 8.0f);
		whiteMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr blackMat = ShaderMaterial::Create();
		blackMat->Shader = shader;
		blackMat->Set("s_Diffuse", blackChecker);
		blackMat->Set("s_Specular", noSpec);
		blackMat->Set("u_Shininess", 8.0f);
		blackMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr clearMat = ShaderMaterial::Create();
		clearMat->Shader = shader;
		clearMat->Set("s_Diffuse", texture2);
		clearMat->Set("u_Shininess", 8.0f);




		GameObject obj1 = scene->CreateEntity("Ground");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/plane.obj");
			obj1.emplace<RendererComponent>().SetMesh(vao).SetMaterial(checkerMat);
			obj1.get<Transform>().SetLocalScale(0.5f, 0.5f, 0.5f);
			obj1.get<Transform>().SetLocalPosition(0.0f, 0.0f, 0.0f);
		}

		GameObject obj2 = scene->CreateEntity("monkey_quads");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/monkey_quads.obj");
			obj2.emplace<RendererComponent>().SetMesh(vao).SetMaterial(stoneMat);
			obj2.get<Transform>().SetLocalPosition(100.0f, 100.0f, 2.0f);
			obj2.get<Transform>().SetLocalRotation(0.0f, 0.0f, -90.0f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj2);
		}

		GameObject checker2 = scene->CreateEntity("Checker 2");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Checkers.obj");
			checker2.emplace<RendererComponent>().SetMesh(vao).SetMaterial(whiteMat);
			checker2.get<Transform>().SetLocalPosition(4.0f, 4.0f, 0.0f);
			checker2.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
		}
		GameObject checker3 = scene->CreateEntity("Checker 3");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Checkers.obj");
			checker3.emplace<RendererComponent>().SetMesh(vao).SetMaterial(blackMat);
			checker3.get<Transform>().SetLocalPosition(-4.0f, -4.0f, 0.0f);
			checker3.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
		}
		GameObject checker4 = scene->CreateEntity("Checker 4");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Checkers.obj");
			checker4.emplace<RendererComponent>().SetMesh(vao).SetMaterial(blackMat);
			checker4.get<Transform>().SetLocalPosition(4.0f, -4.0f, 0.0f);
			checker4.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
		}
		GameObject checker5 = scene->CreateEntity("Checker 5");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Checkers.obj");
			checker5.emplace<RendererComponent>().SetMesh(vao).SetMaterial(whiteMat);
			checker5.get<Transform>().SetLocalPosition(-4.0f, 4.0f, 0.0f);
			checker5.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
		}
		GameObject checker6 = scene->CreateEntity("Checker 6");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Checkers.obj");
			checker6.emplace<RendererComponent>().SetMesh(vao).SetMaterial(blackMat);
			checker6.get<Transform>().SetLocalPosition(-8.0f, -8.0f, 0.0f);
			checker6.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
		}
		GameObject checker7 = scene->CreateEntity("Checker 7");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Checkers.obj");
			checker7.emplace<RendererComponent>().SetMesh(vao).SetMaterial(whiteMat);
			checker7.get<Transform>().SetLocalPosition(8.0f, 8.0f, 0.0f);
			checker7.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
		}
		GameObject checker8 = scene->CreateEntity("Checker 8");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Checkers.obj");
			checker8.emplace<RendererComponent>().SetMesh(vao).SetMaterial(whiteMat);
			checker8.get<Transform>().SetLocalPosition(0.0f, 8.0f, 0.0f);
			checker8.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
		}
		GameObject checker9 = scene->CreateEntity("Checker 9");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Checkers.obj");
			checker9.emplace<RendererComponent>().SetMesh(vao).SetMaterial(blackMat);
			checker9.get<Transform>().SetLocalPosition(0.0f, -8.0f, 0.0f);
			checker9.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
		}
		GameObject checker10 = scene->CreateEntity("Checker 10");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Checkers.obj");
			checker10.emplace<RendererComponent>().SetMesh(vao).SetMaterial(whiteMat);
			checker10.get<Transform>().SetLocalPosition(-8.0f, 8.0f, 0.0f);
			checker10.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
		}
		GameObject checker11 = scene->CreateEntity("Checker 11");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Checkers.obj");
			checker11.emplace<RendererComponent>().SetMesh(vao).SetMaterial(blackMat);
			checker11.get<Transform>().SetLocalPosition(8.0f, -8.0f, 0.0f);
			checker11.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
		}

		std::vector<glm::vec2> allAvoidAreasFrom = { glm::vec2(-4.0f, -4.0f) };
		std::vector<glm::vec2> allAvoidAreasTo = { glm::vec2(4.0f, 4.0f) };

		std::vector<glm::vec2> rockAvoidAreasFrom = { glm::vec2(-3.0f, -3.0f), glm::vec2(-19.0f, -19.0f), glm::vec2(5.0f, -19.0f),
														glm::vec2(-19.0f, 5.0f), glm::vec2(-19.0f, -19.0f) };
		std::vector<glm::vec2> rockAvoidAreasTo = { glm::vec2(3.0f, 3.0f), glm::vec2(19.0f, -5.0f), glm::vec2(19.0f, 19.0f),
														glm::vec2(19.0f, 19.0f), glm::vec2(-5.0f, 19.0f) };
		glm::vec2 spawnFromHere = glm::vec2(-19.0f, -19.0f);
		glm::vec2 spawnToHere = glm::vec2(19.0f, 19.0f);

		// Create an object to be our camera
		GameObject cameraObject = scene->CreateEntity("Camera");
		{
			cameraObject.get<Transform>().SetLocalPosition(0, 3, 3).LookAt(glm::vec3(0, 0, 0));

			// We'll make our camera a component of the camera object
			Camera& camera = cameraObject.emplace<Camera>();// Camera::Create();
			camera.SetPosition(glm::vec3(0, 3, 3));
			camera.SetUp(glm::vec3(0, 0, 1));
			camera.LookAt(glm::vec3(0));
			camera.SetFovDegrees(90.0f); // Set an initial FOV
			camera.SetOrthoHeight(3.0f);
			BehaviourBinding::Bind<CameraControlBehaviour>(cameraObject);
		}

		int width, height;
		glfwGetWindowSize(BackendHandler::window, &width, &height);

		GameObject framebufferObject = scene->CreateEntity("Basic Effect");
		{
			basicEffect = &framebufferObject.emplace<PostEffect>();
			basicEffect->Init(width, height);
		}

		GameObject sepiaEffectObject = scene->CreateEntity("Sepia Effect");
		{
			sepiaEffect = &sepiaEffectObject.emplace<SepiaEffect>();
			sepiaEffect->Init(width, height);
		}
		effects.push_back(sepiaEffect);

		GameObject greyscaleEffectObject = scene->CreateEntity("Greyscale Effect");
		{
			greyscaleEffect = &greyscaleEffectObject.emplace<GreyscaleEffect>();
			greyscaleEffect->Init(width, height);
		}
		effects.push_back(greyscaleEffect);

		GameObject colorCorrectEffectObject = scene->CreateEntity("Color Correct Effect");
		{
			colorCorrectEffect = &colorCorrectEffectObject.emplace<ColorCorrectEffect>();
			colorCorrectEffect->Init(width, height);
		}
		effects.push_back(colorCorrectEffect);

		GameObject bloomEffectObject = scene->CreateEntity("Bloom Effect");
		{
			bloomEffect = &bloomEffectObject.emplace<BloomEffect>();
			bloomEffect->Init(width, height);
		}
		effects.push_back(bloomEffect);

#pragma endregion 
		//////////////////////////////////////////////////////////////////////////////////////////

		/////////////////////////////////// SKYBOX ///////////////////////////////////////////////
		{
			// Load our shaders
			Shader::sptr skybox = std::make_shared<Shader>();
			skybox->LoadShaderPartFromFile("shaders/skybox-shader.vert.glsl", GL_VERTEX_SHADER);
			skybox->LoadShaderPartFromFile("shaders/skybox-shader.frag.glsl", GL_FRAGMENT_SHADER);
			skybox->Link();

			ShaderMaterial::sptr skyboxMat = ShaderMaterial::Create();
			skyboxMat->Shader = skybox;
			skyboxMat->Set("s_Environment", environmentMap);
			skyboxMat->Set("u_EnvironmentRotation", glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0))));
			skyboxMat->RenderLayer = 100;

			MeshBuilder<VertexPosNormTexCol> mesh;
			MeshFactory::AddIcoSphere(mesh, glm::vec3(0.0f), 1.0f);
			MeshFactory::InvertFaces(mesh);
			VertexArrayObject::sptr meshVao = mesh.Bake();

			GameObject skyboxObj = scene->CreateEntity("skybox");
			skyboxObj.get<Transform>().SetLocalPosition(0.0f, 0.0f, 0.0f);
			skyboxObj.get_or_emplace<RendererComponent>().SetMesh(meshVao).SetMaterial(skyboxMat);
		}
		////////////////////////////////////////////////////////////////////////////////////////


		// We'll use a vector to store all our key press events for now (this should probably be a behaviour eventually)
		std::vector<KeyPressWatcher> keyToggles;
		{
			// This is an example of a key press handling helper. Look at InputHelpers.h an .cpp to see
			// how this is implemented. Note that the ampersand here is capturing the variables within
			// the scope. If you wanted to do some method on the class, your best bet would be to give it a method and
			// use std::bind
			keyToggles.emplace_back(GLFW_KEY_T, [&]() { cameraObject.get<Camera>().ToggleOrtho(); });

			controllables.push_back(obj2);

			keyToggles.emplace_back(GLFW_KEY_KP_ADD, [&]() {
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = false;
				selectedVao++;
				if (selectedVao >= controllables.size())
					selectedVao = 0;
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = true;
				});
			keyToggles.emplace_back(GLFW_KEY_KP_SUBTRACT, [&]() {
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = false;
				selectedVao--;
				if (selectedVao < 0)
					selectedVao = controllables.size() - 1;
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = true;
				});

			keyToggles.emplace_back(GLFW_KEY_Y, [&]() {
				auto behaviour = BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao]);
				behaviour->Relative = !behaviour->Relative;
				});
		}

		// Initialize our timing instance and grab a reference for our use
		Timing& time = Timing::Instance();
		time.LastFrame = glfwGetTime();

		///// Game loop /////
		while (!glfwWindowShouldClose(BackendHandler::window)) {
			glfwPollEvents();


			lightTime += time.DeltaTime;

			shader->SetUniform("u_Time", lightTime);

			if (forwards)
				t += time.DeltaTime / totalTime;
			else
				t -= time.DeltaTime / totalTime;

			if (t < 0.0f)
				t = 0.0f;

			if (t > 1.0f)
				t = 1.0f;

			if (t >= 1.0f || t <= 0.0f)
				forwards = !forwards;

			currentPos = glm::mix(point1, point2, t);
			checker2CurrentPos = glm::mix(checker2Point1, checker2Point2, t);
			checker3CurrentPos = glm::mix(checker3Point1, checker3Point2, t);

			checker2.get<Transform>().SetLocalPosition(checker2CurrentPos.x, checker2CurrentPos.y, checker2CurrentPos.z);
			checker3.get<Transform>().SetLocalPosition(checker3CurrentPos.x, checker3CurrentPos.y, checker3CurrentPos.z);

			shader->SetUniform("u_Position", currentPos);

			// Update the timing
			time.CurrentFrame = glfwGetTime();
			time.DeltaTime = static_cast<float>(time.CurrentFrame - time.LastFrame);

			time.DeltaTime = time.DeltaTime > 1.0f ? 1.0f : time.DeltaTime;

			// Update our FPS tracker data
			fpsBuffer[frameIx] = 1.0f / time.DeltaTime;
			frameIx++;
			if (frameIx >= 128)
				frameIx = 0;

			if (glfwGetKey(BackendHandler::window, GLFW_KEY_1) == GLFW_PRESS)
			{
				lightNum = 1;
			}
			if (glfwGetKey(BackendHandler::window, GLFW_KEY_2) == GLFW_PRESS)
			{
				lightNum = 2;
			}
			if (glfwGetKey(BackendHandler::window, GLFW_KEY_3) == GLFW_PRESS)
			{
				lightNum = 3;
			}
			if (glfwGetKey(BackendHandler::window, GLFW_KEY_4) == GLFW_PRESS)
			{
				lightNum = 4;
			}
			if (glfwGetKey(BackendHandler::window, GLFW_KEY_5) == GLFW_PRESS)
			{
				lightNum = 5;
			}

			shader->SetUniform("u_LightNum", lightNum);

			// We'll make sure our UI isn't focused before we start handling input for our game
			if (!ImGui::IsAnyWindowFocused()) {
				// We need to poll our key watchers so they can do their logic with the GLFW state
				// Note that since we want to make sure we don't copy our key handlers, we need a const
				// reference!
				for (const KeyPressWatcher& watcher : keyToggles) {
					watcher.Poll(BackendHandler::window);
				}
			}

			// Iterate over all the behaviour binding components
			scene->Registry().view<BehaviourBinding>().each([&](entt::entity entity, BehaviourBinding& binding) {
				// Iterate over all the behaviour scripts attached to the entity, and update them in sequence (if enabled)
				for (const auto& behaviour : binding.Behaviours) {
					if (behaviour->Enabled) {
						behaviour->Update(entt::handle(scene->Registry(), entity));
					}
				}
				});

			if (isTextured)
			{
				obj1.get<RendererComponent>().SetMaterial(checkerMat);
				obj2.get<RendererComponent>().SetMaterial(stoneMat);
				checker2.get<RendererComponent>().SetMaterial(whiteMat);
				checker3.get<RendererComponent>().SetMaterial(blackMat);
				checker4.get<RendererComponent>().SetMaterial(blackMat);
				checker5.get<RendererComponent>().SetMaterial(whiteMat);
				checker6.get<RendererComponent>().SetMaterial(blackMat);
				checker7.get<RendererComponent>().SetMaterial(whiteMat);
				checker8.get<RendererComponent>().SetMaterial(whiteMat);
				checker9.get<RendererComponent>().SetMaterial(blackMat);
				checker10.get<RendererComponent>().SetMaterial(whiteMat);
				checker11.get<RendererComponent>().SetMaterial(blackMat);
			}
			else
			{
				obj1.get<RendererComponent>().SetMaterial(clearMat);
				obj2.get<RendererComponent>().SetMaterial(clearMat);
				checker2.get<RendererComponent>().SetMaterial(clearMat);
				checker3.get<RendererComponent>().SetMaterial(clearMat);
				checker4.get<RendererComponent>().SetMaterial(clearMat);
				checker5.get<RendererComponent>().SetMaterial(clearMat);
				checker6.get<RendererComponent>().SetMaterial(clearMat);
				checker7.get<RendererComponent>().SetMaterial(clearMat);
				checker8.get<RendererComponent>().SetMaterial(clearMat);
				checker9.get<RendererComponent>().SetMaterial(clearMat);
				checker10.get<RendererComponent>().SetMaterial(clearMat);
				checker11.get<RendererComponent>().SetMaterial(clearMat);
			}

			// Clear the screen
			basicEffect->Clear();

			for (int i = 0; i < effects.size(); i++)
			{
				effects[i]->Clear();
			}


			glClearColor(0.08f, 0.17f, 0.31f, 1.0f);
			glEnable(GL_DEPTH_TEST);
			glClearDepth(1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Update all world matrices for this frame
			scene->Registry().view<Transform>().each([](entt::entity entity, Transform& t) {
				t.UpdateWorldMatrix();
				});

			// Grab out camera info from the camera object
			Transform& camTransform = cameraObject.get<Transform>();
			glm::mat4 view = glm::inverse(camTransform.LocalTransform());
			glm::mat4 projection = cameraObject.get<Camera>().GetProjection();
			glm::mat4 viewProjection = projection * view;

			// Sort the renderers by shader and material, we will go for a minimizing context switches approach here,
			// but you could for instance sort front to back to optimize for fill rate if you have intensive fragment shaders
			renderGroup.sort<RendererComponent>([](const RendererComponent& l, const RendererComponent& r) {
				// Sort by render layer first, higher numbers get drawn last
				if (l.Material->RenderLayer < r.Material->RenderLayer) return true;
				if (l.Material->RenderLayer > r.Material->RenderLayer) return false;

				// Sort by shader pointer next (so materials using the same shader run sequentially where possible)
				if (l.Material->Shader < r.Material->Shader) return true;
				if (l.Material->Shader > r.Material->Shader) return false;

				// Sort by material pointer last (so we can minimize switching between materials)
				if (l.Material < r.Material) return true;
				if (l.Material > r.Material) return false;

				return false;
				});

			// Start by assuming no shader or material is applied
			Shader::sptr current = nullptr;
			ShaderMaterial::sptr currentMat = nullptr;

			basicEffect->BindBuffer(0);

			// Iterate over the render group components and draw them
			renderGroup.each([&](entt::entity e, RendererComponent& renderer, Transform& transform) {
				// If the shader has changed, set up it's uniforms
				if (current != renderer.Material->Shader) {
					current = renderer.Material->Shader;
					current->Bind();
					BackendHandler::SetupShaderForFrame(current, view, projection);
				}
				// If the material has changed, apply it
				if (currentMat != renderer.Material) {
					currentMat = renderer.Material;
					currentMat->Apply();
				}
				// Render the mesh
				BackendHandler::RenderVAO(renderer.Material->Shader, renderer.Mesh, viewProjection, transform);
				});

			basicEffect->UnbindBuffer();

			effects[activeEffect]->ApplyEffect(basicEffect);

			effects[activeEffect]->DrawToScreen();

			// Draw our ImGui content
			BackendHandler::RenderImGui();

			scene->Poll();
			glfwSwapBuffers(BackendHandler::window);
			time.LastFrame = time.CurrentFrame;
		}

		// Nullify scene so that we can release references
		Application::Instance().ActiveScene = nullptr;
		//Clean up the environment generator so we can release references
		BackendHandler::ShutdownImGui();
	}

	// Clean up the toolkit logger so we don't leak memory
	Logger::Uninitialize();
	return 0;
}
