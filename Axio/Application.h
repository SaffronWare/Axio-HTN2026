#include <glad/glad.h>
#include <glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_node_editor.h>

#include <stdexcept>


struct Application
{
private:
	GLFWwindow* window;
	ax::NodeEditor::EditorContext* m_Context;


public:
	Application();
	~Application();

	void start();

	void InitGlfw();
	void InitImGui();

	void Draw();
	void DrawViewport();
	void DrawNodeEditor();
};