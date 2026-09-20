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
	GLuint texture;
	GLuint rbo;
	GLuint fbo;
	GLuint triangleVAO = 0;
	GLuint triangleProgram = 0;

	int viewportWidth = 800;
	int viewportHeight = 600;

public:
	Application();
	~Application();

	void start();

	void InitGlfw();
	void InitImGui();

	void CreateViewportFramebuffer();
	void RecViewportTex();

	void Draw();
	void DrawViewport();
	void DrawNodeEditor();
	void DrawNodeCatalogue();
};