#include "Application.h"

namespace ed = ax::NodeEditor;

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
	}
}

Application::Application()
{
	InitGlfw();
	InitImGui();
}

Application::~Application()
{
	ed::DestroyEditor(m_Context);

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


void Application::InitGlfw() {
	glfwInit();
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);

	window = glfwCreateWindow(3 * 640, 3 * 480, "Axio", NULL, NULL);
	if (!window)
	{
		throw std::runtime_error("Failed to create window!\n");
		glfwTerminate();

	}

	glfwMakeContextCurrent(window);



	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		throw std::runtime_error("Failed to intialize glad!\n");
		glfwTerminate();

	}

	glViewport(0, 0, 3 * 640,  3 * 480);
}


void Application::InitImGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	ImGui::StyleColorsDark();


	// Setup scaling
	ImGuiStyle& style = ImGui::GetStyle();

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

	ImGui::Render();

	glClearColor(0.2f, 0.3f, 0.3f, 1.0f); 
	glClear(GL_COLOR_BUFFER_BIT); 
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); 
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) 
	{ 
		GLFWwindow* backup_current_context = glfwGetCurrentContext(); 
		ImGui::UpdatePlatformWindows(); 
		ImGui::RenderPlatformWindowsDefault(); glfwMakeContextCurrent(backup_current_context); }
}


void Application::DrawViewport() {
	if (ImGui::Begin("Viewport"))
	{
		ImGui::End();
	}
}


void Application::DrawNodeEditor()
{
	if (ImGui::Begin("Node Editor")) {

		ed::SetCurrentEditor(m_Context);
		ed::Begin("My Editor", ImVec2(0.0, 0.0f));
		int uniqueId = 1;

		ed::BeginNode(uniqueId++);
		ImGui::Text("Node A");
		ed::BeginPin(uniqueId++, ed::PinKind::Input);
		ImGui::Text("-> In");
		ed::EndPin();
		ImGui::SameLine();
		ed::BeginPin(uniqueId++, ed::PinKind::Output);
		ImGui::Text("Out ->");
		ed::EndPin();
		ed::EndNode();
		ed::End();
		ed::SetCurrentEditor(nullptr);

		ImGui::End();
	}
}
