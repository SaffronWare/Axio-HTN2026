#include "Application.h"

namespace ed = ax::NodeEditor;

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
	}
}

static std::string ReadFile(const char* path)
{
	std::ifstream in(path, std::ios::in | std::ios::binary);

	if (!in)
	{
		throw std::runtime_error(
			std::string("Failed to open file: ") + path
		);
	}

	std::ostringstream ss;
	ss << in.rdbuf();

	return ss.str();
}

static GLuint CompileShader(GLenum type, const std::string& source)
{
	GLuint shader = glCreateShader(type);

	const char* src = source.c_str();

	glShaderSource(
		shader,
		1,
		&src,
		nullptr
	);

	glCompileShader(shader);

	GLint success = 0;

	glGetShaderiv(
		shader,
		GL_COMPILE_STATUS,
		&success
	);

	if (!success)
	{
		GLint logLength = 0;

		glGetShaderiv(
			shader,
			GL_INFO_LOG_LENGTH,
			&logLength
		);

		std::string log(logLength, '\0');

		glGetShaderInfoLog(
			shader,
			logLength,
			nullptr,
			log.data()
		);

		glDeleteShader(shader);

		throw std::runtime_error(
			std::string("Shader compile failed:\n") + log
		);
	}

	return shader;
}

static GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader)
{
	GLuint program = glCreateProgram();

	glAttachShader(
		program,
		vertexShader
	);

	glAttachShader(
		program,
		fragmentShader
	);

	glLinkProgram(program);

	GLint success = 0;

	glGetProgramiv(
		program,
		GL_LINK_STATUS,
		&success
	);

	if (!success)
	{
		GLint logLength = 0;

		glGetProgramiv(
			program,
			GL_INFO_LOG_LENGTH,
			&logLength
		);

		std::string log(logLength, '\0');

		glGetProgramInfoLog(
			program,
			logLength,
			nullptr,
			log.data()
		);

		glDeleteProgram(program);

		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);

		throw std::runtime_error(
			std::string("Program link failed:\n") + log
		);
	}

	glDetachShader(
		program,
		vertexShader
	);

	glDetachShader(
		program,
		fragmentShader
	);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return program;
}

static GLuint CreateProgramFromFiles(
	const char* vertexPath,
	const char* fragmentPath
)
{
	std::string vertexSource =
		ReadFile(vertexPath);

	std::string fragmentSource =
		ReadFile(fragmentPath);

	GLuint vertexShader =
		CompileShader(
			GL_VERTEX_SHADER,
			vertexSource
		);

	GLuint fragmentShader =
		CompileShader(
			GL_FRAGMENT_SHADER,
			fragmentSource
		);

	return LinkProgram(
		vertexShader,
		fragmentShader
	);
}

Application::Application()
{
	InitGlfw();

	CreateViewportFramebuffer();

	glGenVertexArrays(
		1,
		&triangleVAO
	);

	triangleProgram =
		CreateProgramFromFiles(
			"C:/Users/aryan/source/repos/Axio/Axio/shaders/vert.glsl",
			"C:/Users/aryan/source/repos/Axio/Axio/shaders/frag.glsl"
		);

	InitImGui();
}

Application::~Application()
{
	glDeleteProgram(
		triangleProgram
	);

	glDeleteVertexArrays(
		1,
		&triangleVAO
	);

	glDeleteTextures(
		1,
		&texture
	);

	glDeleteFramebuffers(
		1,
		&fbo
	);

	ed::DestroyEditor(
		m_Context
	);

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();

	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();
}

void Application::start()
{
	while (!glfwWindowShouldClose(window))
	{
		processInput(window);

		Draw();

		glfwPollEvents();
		glfwSwapBuffers(window);
	}
}

void Application::CreateViewportFramebuffer()
{
	glGenFramebuffers(
		1,
		&fbo
	);

	glBindFramebuffer(
		GL_FRAMEBUFFER,
		fbo
	);

	glGenTextures(
		1,
		&texture
	);

	glBindTexture(
		GL_TEXTURE_2D,
		texture
	);

	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGB,
		viewportWidth,
		viewportHeight,
		0,
		GL_RGB,
		GL_UNSIGNED_BYTE,
		nullptr
	);

	glTexParameteri(
		GL_TEXTURE_2D,
		GL_TEXTURE_MIN_FILTER,
		GL_LINEAR
	);

	glTexParameteri(
		GL_TEXTURE_2D,
		GL_TEXTURE_MAG_FILTER,
		GL_LINEAR
	);

	glFramebufferTexture2D(
		GL_FRAMEBUFFER,
		GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D,
		texture,
		0
	);

	if (
		glCheckFramebufferStatus(GL_FRAMEBUFFER)
		!= GL_FRAMEBUFFER_COMPLETE
		)
	{
		throw std::runtime_error(
			"Framebuffer incomplete"
		);
	}

	glBindTexture(
		GL_TEXTURE_2D,
		0
	);

	glBindFramebuffer(
		GL_FRAMEBUFFER,
		0
	);
}

void Application::InitGlfw()
{
	glfwInit();

	glfwWindowHint(
		GLFW_OPENGL_PROFILE,
		GLFW_OPENGL_CORE_PROFILE
	);

	glfwWindowHint(
		GLFW_CONTEXT_VERSION_MAJOR,
		4
	);

	glfwWindowHint(
		GLFW_CONTEXT_VERSION_MINOR,
		6
	);

	glfwWindowHint(
		GLFW_SAMPLES,
		4
	);

	window =
		glfwCreateWindow(
			3 * 640,
			3 * 480,
			"Axio",
			NULL,
			NULL
		);

	if (!window)
	{
		glfwTerminate();

		throw std::runtime_error(
			"Failed to create window!\n"
		);
	}

	glfwMakeContextCurrent(
		window
	);

	if (
		!gladLoadGLLoader(
			(GLADloadproc)glfwGetProcAddress
		)
		)
	{
		glfwTerminate();

		throw std::runtime_error(
			"Failed to initialize glad!\n"
		);
	}

	glEnable(
		GL_MULTISAMPLE
	);

	glViewport(
		0,
		0,
		3 * 640,
		3 * 480
	);
}

void Application::InitImGui()
{
	IMGUI_CHECKVERSION();

	ImGui::CreateContext();

	ImGuiIO& io =
		ImGui::GetIO();

	io.ConfigFlags |=
		ImGuiConfigFlags_NavEnableKeyboard;

	io.ConfigFlags |=
		ImGuiConfigFlags_NavEnableGamepad;

	io.ConfigFlags |=
		ImGuiConfigFlags_DockingEnable;

	io.ConfigFlags |=
		ImGuiConfigFlags_ViewportsEnable;

	io.Fonts->AddFontFromFileTTF(
		"C:/Windows/Fonts/segoeui.ttf",
		20.0f
	);

	ImGui::StyleColorsDark();

	ImGuiStyle& style =
		ImGui::GetStyle();

	style.AntiAliasedLines = true;
	style.AntiAliasedLinesUseTex = true;
	style.AntiAliasedFill = true;

	style.FrameRounding = 10.0f;
	style.TabRounding = 10.0f;

	ImGui_ImplGlfw_InitForOpenGL(
		window,
		true
	);

	ImGui_ImplOpenGL3_Init();

	ax::NodeEditor::Config config;

	config.SettingsFile =
		"Simple.json";

	m_Context =
		ax::NodeEditor::CreateEditor(
			&config
		);
}

void Application::Draw()
{
	ImGuiIO& io =
		ImGui::GetIO();

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();

	ImGui::NewFrame();

	ImGui::DockSpaceOverViewport();

	DrawViewport();
	DrawNodeEditor();
	DrawNodeCatalogue();

	if (
		viewportWidth > 0 &&
		viewportHeight > 0
		)
	{
		glBindFramebuffer(
			GL_FRAMEBUFFER,
			fbo
		);

		glViewport(
			0,
			0,
			viewportWidth,
			viewportHeight
		);

		glClearColor(
			0.02f,
			0.02f,
			0.025f,
			1.0f
		);

		glClear(
			GL_COLOR_BUFFER_BIT
		);

		glUseProgram(
			triangleProgram
		);

		glUniform2f(
			glGetUniformLocation(
				triangleProgram,
				"resolution"
			),
			static_cast<float>(
				viewportWidth
				),
			static_cast<float>(
				viewportHeight
				)
		);

		glUniform3f(
			glGetUniformLocation(
				triangleProgram,
				"cameraPosition"
			),
			cameraPosition[0],
			cameraPosition[1],
			cameraPosition[2]
		);

		glUniform3f(
			glGetUniformLocation(
				triangleProgram,
				"cameraFront"
			),
			cameraFront[0],
			cameraFront[1],
			cameraFront[2]
		);

		glUniform3f(
			glGetUniformLocation(
				triangleProgram,
				"cameraRight"
			),
			cameraRight[0],
			cameraRight[1],
			cameraRight[2]
		);

		glUniform3f(
			glGetUniformLocation(
				triangleProgram,
				"cameraUp"
			),
			cameraUp[0],
			cameraUp[1],
			cameraUp[2]
		);

		glUniform1f(
			glGetUniformLocation(
				triangleProgram,
				"focalLength"
			),
			focalLength
		);

		glBindVertexArray(
			triangleVAO
		);

		glDrawArrays(
			GL_TRIANGLES,
			0,
			3
		);

		glBindVertexArray(
			0
		);

		glUseProgram(
			0
		);

		glBindFramebuffer(
			GL_FRAMEBUFFER,
			0
		);
	}

	ImGui::Render();

	int width = 0;
	int height = 0;

	glfwGetFramebufferSize(
		window,
		&width,
		&height
	);

	glViewport(
		0,
		0,
		width,
		height
	);

	glClearColor(
		0.2f,
		0.3f,
		0.3f,
		1.0f
	);

	glClear(
		GL_COLOR_BUFFER_BIT
	);

	ImGui_ImplOpenGL3_RenderDrawData(
		ImGui::GetDrawData()
	);

	if (
		io.ConfigFlags &
		ImGuiConfigFlags_ViewportsEnable
		)
	{
		GLFWwindow* backupCurrentContext =
			glfwGetCurrentContext();

		ImGui::UpdatePlatformWindows();

		ImGui::RenderPlatformWindowsDefault();

		glfwMakeContextCurrent(
			backupCurrentContext
		);
	}
}

void Application::DrawViewport()
{
	ImGui::Begin(
		"Viewport"
	);

	ImVec2 size =
		ImGui::GetContentRegionAvail();

	int newWidth =
		static_cast<int>(
			size.x
			);

	int newHeight =
		static_cast<int>(
			size.y
			);

	if (
		newWidth > 0 &&
		newHeight > 0 &&
		(
			newWidth != viewportWidth ||
			newHeight != viewportHeight
			)
		)
	{
		viewportWidth =
			newWidth;

		viewportHeight =
			newHeight;

		glBindTexture(
			GL_TEXTURE_2D,
			texture
		);

		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RGB,
			viewportWidth,
			viewportHeight,
			0,
			GL_RGB,
			GL_UNSIGNED_BYTE,
			nullptr
		);

		glBindTexture(
			GL_TEXTURE_2D,
			0
		);
	}

	if (
		size.x > 0.0f &&
		size.y > 0.0f
		)
	{
		ImGui::Image(
			(ImTextureID)(intptr_t)texture,
			size,
			ImVec2(0, 1),
			ImVec2(1, 0)
		);
	}

	ImGui::End();
}

void Application::DrawNodeEditor()
{
	bool visible = ImGui::Begin("Node Editor");

	if (!visible)
	{
		ImGui::End();
		return;
	}

	ed::SetCurrentEditor(m_Context);

	ed::Begin(
		"My Editor",
		ImVec2(0.0f, 0.0f)
	);

	for (auto& node : nodes)
	{
		ed::BeginNode(node.uniqueId);

		ImGui::Text(
			"%s",
			node.name.c_str()
		);

		for (auto& input : node.inputs)
		{
			ed::BeginPin(
				input.uniqueId,
				ed::PinKind::Input
			);

			ImGui::Text(
				"-> %s",
				input.name.c_str()
			);

			ed::EndPin();
		}

		for (auto& output : node.outputs)
		{
			ed::BeginPin(
				output.uniqueId,
				ed::PinKind::Output
			);

			ImGui::Text(
				"%s ->",
				output.name.c_str()
			);

			ed::EndPin();
		}

		ed::EndNode();
	}

	ImVec2 popupPosition = ImGui::GetMousePos();

	ed::Suspend();

	if (ed::ShowBackgroundContextMenu())
	{
		ImGui::OpenPopup("Create Node");
	}

	ed::Resume();

	ed::Suspend();

	if (ImGui::BeginPopup("Create Node"))
	{
		if (ImGui::MenuItem("Sphere"))
		{
			Node* node = AddNode(
				"Sphere",
				{
					{ "Position", PinData::Vec3 },
					{ "Radius", PinData::Float }
				},
				{
					{ "Distance", PinData::Float }
				}
			);

			ed::SetNodePosition(
				node->uniqueId,
				ed::ScreenToCanvas(popupPosition)
			);
		}

		if (ImGui::MenuItem("Add"))
		{
			Node* node = AddNode(
				"Add",
				{
					{ "A", PinData::Float },
					{ "B", PinData::Float }
				},
				{
					{ "Result", PinData::Float }
				}
			);

			ed::SetNodePosition(
				node->uniqueId,
				ed::ScreenToCanvas(popupPosition)
			);
		}

		if (ImGui::MenuItem("Output"))
		{
			Node* node = AddNode(
				"Output",
				{
					{ "Distance", PinData::Float }
				},
				{}
			);

			ed::SetNodePosition(
				node->uniqueId,
				ed::ScreenToCanvas(popupPosition)
			);
		}

		ImGui::EndPopup();
	}

	ed::Resume();

	ed::End();

	ed::SetCurrentEditor(nullptr);

	ImGui::End();
}

void Application::DrawNodeCatalogue()
{
}

int Application::GetNextNodeEditorId()
{
	return nextNodeEditorId++;
}

Node* Application::AddNode(
	const std::string& name,
	const std::vector<std::pair<std::string, PinData>>& inputs,
	const std::vector<std::pair<std::string, PinData>>& outputs
)
{
	nodes.push_back(Node{});

	Node& node = nodes.back();

	node.uniqueId = ed::NodeId(
		GetNextNodeEditorId()
	);

	node.name = name;

	for (const auto& input : inputs)
	{
		Pin pin;

		pin.uniqueId =
			ed::PinId(
				GetNextNodeEditorId()
			);

		pin.node = &node;
		pin.name = input.first;
		pin.type = PinType::Input;
		pin.data = input.second;

		node.inputs.push_back(
			pin
		);
	}

	for (const auto& output : outputs)
	{
		Pin pin;

		pin.uniqueId =
			ed::PinId(
				GetNextNodeEditorId()
			);

		pin.node = &node;
		pin.name = output.first;
		pin.type = PinType::Output;
		pin.data = output.second;

		node.outputs.push_back(
			pin
		);
	}

	return &node;
}

Pin* Application::FindPin(ed::PinId id)
{
	for (auto& node : nodes)
	{
		for (auto& pin : node.inputs)
		{
			if (pin.uniqueId == id)
				return &pin;
		}

		for (auto& pin : node.outputs)
		{
			if (pin.uniqueId == id)
				return &pin;
		}
	}

	return nullptr;
}

bool Application::CanCreateLink(
	Pin* a,
	Pin* b
)
{
	if (!a || !b)
		return false;

	if (a == b)
		return false;

	if (a->node == b->node)
		return false;

	if (a->type == b->type)
		return false;

	if (a->data != b->data)
		return false;

	return true;
}