#pragma once

#include <glad/glad.h>
#include <glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_node_editor.h>

#include "NodeEditor.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

class Application
{
public:
    Application();
    ~Application();

    void start();

private:
    struct GraphCompileContext
    {
        std::ostringstream code;

        std::unordered_map<const Pin*, std::string> outputExpressions;
        std::unordered_map<const Node*, int> nodeIndices;

        std::unordered_set<const Node*> emitted;
        std::unordered_set<const Node*> visiting;

        int nextNodeIndex = 0;
    };

private:
    void InitGlfw();
    void InitImGui();
    void CreateViewportFramebuffer();

    void Draw();
    void DrawViewport();
    void DrawNodeEditor();
    void DrawNodeCatalogue();

    int GetNextNodeEditorId();

    Node* AddNode(
        NodeType type,
        const std::string& name,
        const std::vector<std::pair<std::string, PinData>>& inputs,
        const std::vector<std::pair<std::string, PinData>>& outputs
    );

    Node* SpawnNode(NodeType type);

    Pin* FindPin(ed::PinId id);
    Pin* FindConnectedOutput(ed::PinId inputPinId);

    bool IsInputConnected(const Pin& pin) const;
    bool CanCreateLink(Pin* a, Pin* b);

    bool DrawPinValueEditor(Pin& pin);

    void MarkGraphDirty();
    void RecompileGraphShader();

    std::string BuildGeneratedFragmentShader();
    std::string BuildSceneFunction();

    std::string CompileInputPin(
        const Pin& input,
        GraphCompileContext& context
    );

    std::string CompileOutputPin(
        Pin* output,
        GraphCompileContext& context
    );

    void CompileNode(
        Node* node,
        GraphCompileContext& context
    );

    std::string LiteralForPin(const Pin& pin) const;
    std::string GlslFloat(float value) const;

private:
    GLFWwindow* window = nullptr;

    ed::EditorContext* m_Context = nullptr;

    std::vector<Node> nodes;
    std::vector<Link> links;

    Node* inputNode = nullptr;
    Node* outputNode = nullptr;

    int nextNodeEditorId = 1;

    int viewportWidth = 800;
    int viewportHeight = 600;

    float cameraPosition[3] = { 0.0f, 1.2f, 4.0f };
    float cameraFront[3] = { 0.0f, -0.15f, -1.0f };
    float cameraRight[3] = { 1.0f, 0.0f, 0.0f };
    float cameraUp[3] = { 0.0f, 1.0f, 0.0f };
    float focalLength = 1.0f;

    GLuint fbo = 0;
    GLuint texture = 0;

    GLuint triangleVAO = 0;
    GLuint triangleProgram = 0;

    bool graphDirty = false;
    double graphDirtySince = 0.0;

    std::string shaderError;

    const std::string vertexShaderPath =
        "C:/Users/aryan/source/repos/Axio/Axio/shaders/vert.glsl";

    const std::string fragmentTemplatePath =
        "C:/Users/aryan/source/repos/Axio/Axio/shaders/frag_template.glsl";

    const std::string generatedFragmentPath =
        "C:/Users/aryan/source/repos/Axio/Axio/shaders/generated_frag.glsl";
};
