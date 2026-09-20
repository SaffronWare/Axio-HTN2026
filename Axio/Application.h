#include <glad/glad.h>
#include <glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_node_editor.h>

#include <stdexcept>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <filesystem>
#include <vector>

#include "NodeEditor.h"




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

	float cameraPosition[3] = { 0.0f, 0.0f, 3.0f };

	float cameraFront[3] = { 0.0f, 0.0f, -1.0f };
	float cameraRight[3] = { 1.0f, 0.0f, 0.0f };
	float cameraUp[3] = { 0.0f, 1.0f, 0.0f };

	float focalLength = 1.0f;

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

	std::vector<Node> nodes;
	std::vector<Link> links;

	int nextNodeEditorId = 1;

	int GetNextNodeEditorId();

	Node* AddNode(
		const std::string& name,
		const std::vector<std::pair<std::string, PinData>>& inputs,
		const std::vector<std::pair<std::string, PinData>>& outputs
	);

	Pin* FindPin(ed::PinId id);

	bool CanCreateLink(Pin* a, Pin* b);
};