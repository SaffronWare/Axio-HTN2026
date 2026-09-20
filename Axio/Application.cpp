#include "Application.h"

namespace ed = ax::NodeEditor;

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
	}
}

static GLuint CompileShader(GLenum type, const char* source)
{
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, nullptr);
	glCompileShader(shader);

	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		char infoLog[1024];
		glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
		throw std::runtime_error(infoLog);
	}

	return shader;
}

static GLuint CreateFullscreenTriangleProgram()
{
	const char* vertexShaderSource = R"(
		#version 460 core

		const vec2 positions[3] = vec2[](
			vec2(-1.0, -1.0),
			vec2( 3.0, -1.0),
			vec2(-1.0,  3.0)
		);

		void main()
		{
			gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
		}
	)";

	const char* fragmentShaderSource = R"(
		#version 460 core

		uniform vec2 resolution;

		out vec4 FragColor;

		void main()
		{
			vec2 uv = gl_FragCoord.xy / resolution;

			FragColor = vec4(
				uv.x,
				uv.y,
				0.4,
				1.0
			);
		}
	)";

	GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
	GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

	GLuint program = glCreateProgram();

	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);

	glLinkProgram(program);

	GLint success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);

	if (!success)
	{
		char infoLog[1024];
		glGetProgramInfoLog(program, 1024, nullptr, infoLog);
		throw std::runtime_error(infoLog);
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return program;
}

Application::Application()
{
	InitGlfw();
	CreateViewportFramebuffer();

	glGenVertexArrays(1, &triangleVAO);
	triangleProgram = CreateFullscreenTriangleProgram();

	InitImGui();
}

Application::~Application()
{
	glDeleteProgram(triangleProgram);
	glDeleteVertexArrays(1, &triangleVAO);

	glDeleteTextures(1, &texture);
	glDeleteFramebuffers(1, &fbo);

	ed::DestroyEditor(m_Context);

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
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

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

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glFramebufferTexture2D(
		GL_FRAMEBUFFER,
		GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D,
		texture,
		0
	);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		throw std::runtime_error("Framebuffer incomplete");
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Application::InitGlfw()
{
	glfwInit();

	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_SAMPLES, 4);

	window = glfwCreateWindow(3 * 640, 3 * 480, "Axio", NULL, NULL);

	if (!window)
	{
		glfwTerminate();
		throw std::runtime_error("Failed to create window!\n");
	}

	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		glfwTerminate();
		throw std::runtime_error("Failed to initialize glad!\n");
	}

	glEnable(GL_MULTISAMPLE);

	glViewport(0, 0, 3 * 640, 3 * 480);
}

void Application::InitImGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	io.Fonts->AddFontFromFileTTF(
		"C:/Windows/Fonts/segoeui.ttf",
		20.0f
	);

	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();

	style.AntiAliasedLines = true;
	style.AntiAliasedLinesUseTex = true;
	style.AntiAliasedFill = true;

	style.FrameRounding = 10.0f;
	style.TabRounding = 10.0f;

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	ax::NodeEditor::Config config;
	config.SettingsFile = "Simple.json";

	m_Context = ax::NodeEditor::CreateEditor(&config);
}

void Application::Draw()
{
	ImGuiIO& io = ImGui::GetIO();

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::DockSpaceOverViewport();

	DrawViewport();
	DrawNodeEditor();
	DrawNodeCatalogue();

	if (viewportWidth > 0 && viewportHeight > 0)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);

		glViewport(
			0,
			0,
			viewportWidth,
			viewportHeight
		);

		glClearColor(0.02f, 0.02f, 0.025f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(triangleProgram);

		GLint resolutionLocation =
			glGetUniformLocation(triangleProgram, "resolution");

		glUniform2f(
			resolutionLocation,
			static_cast<float>(viewportWidth),
			static_cast<float>(viewportHeight)
		);

		glBindVertexArray(triangleVAO);

		glDrawArrays(
			GL_TRIANGLES,
			0,
			3
		);

		glBindVertexArray(0);
		glUseProgram(0);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	ImGui::Render();

	int width;
	int height;

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

	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	ImGui_ImplOpenGL3_RenderDrawData(
		ImGui::GetDrawData()
	);

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		GLFWwindow* backup_current_context =
			glfwGetCurrentContext();

		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();

		glfwMakeContextCurrent(
			backup_current_context
		);
	}
}

void Application::DrawViewport()
{
	ImGui::Begin("Viewport");

	ImVec2 size = ImGui::GetContentRegionAvail();

	int newWidth = static_cast<int>(size.x);
	int newHeight = static_cast<int>(size.y);

	if (
		newWidth > 0 &&
		newHeight > 0 &&
		(
			newWidth != viewportWidth ||
			newHeight != viewportHeight
			)
		)
	{
		viewportWidth = newWidth;
		viewportHeight = newHeight;

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

	if (size.x > 0.0f && size.y > 0.0f)
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

	if (visible)
	{
		ed::SetCurrentEditor(m_Context);

		ed::Begin(
			"My Editor",
			ImVec2(0.0f, 0.0f)
		);

		int uniqueId = 1;

		ed::BeginNode(uniqueId++);

		ImGui::Text("Node A");

		ed::BeginPin(
			uniqueId++,
			ed::PinKind::Input
		);

		ImGui::Text("-> In");

		ed::EndPin();

		ImGui::SameLine();

		ed::BeginPin(
			uniqueId++,
			ed::PinKind::Output
		);

		ImGui::Text("Out ->");

		ed::EndPin();

		ed::EndNode();

		ed::End();

		ed::SetCurrentEditor(nullptr);
	}

	ImGui::End();
}

void Application::DrawNodeCatalogue()
{
}