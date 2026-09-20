#include "Application.h"

#include <algorithm>
#include <cmath>

namespace ed = ax::NodeEditor;

namespace
{
    void processInput(GLFWwindow* window)
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }
    }

    std::string ReadFile(const char* path)
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

    void WriteFile(
        const char* path,
        const std::string& contents
    )
    {
        std::ofstream out(
            path,
            std::ios::out | std::ios::binary | std::ios::trunc
        );

        if (!out)
        {
            throw std::runtime_error(
                std::string("Failed to write file: ") + path
            );
        }

        out << contents;
    }

    GLuint CompileShader(
        GLenum type,
        const std::string& source
    )
    {
        GLuint shader =
            glCreateShader(type);

        const char* src =
            source.c_str();

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

            std::string log(
                static_cast<size_t>(
                    std::max(logLength, 1)
                    ),
                '\0'
            );

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

    GLuint LinkProgram(
        GLuint vertexShader,
        GLuint fragmentShader
    )
    {
        GLuint program =
            glCreateProgram();

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

            std::string log(
                static_cast<size_t>(
                    std::max(logLength, 1)
                    ),
                '\0'
            );

            glGetProgramInfoLog(
                program,
                logLength,
                nullptr,
                log.data()
            );

            glDeleteProgram(program);

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

        return program;
    }

    GLuint CreateProgramFromSources(
        const std::string& vertexSource,
        const std::string& fragmentSource
    )
    {
        GLuint vertexShader = 0;
        GLuint fragmentShader = 0;

        try
        {
            vertexShader =
                CompileShader(
                    GL_VERTEX_SHADER,
                    vertexSource
                );

            fragmentShader =
                CompileShader(
                    GL_FRAGMENT_SHADER,
                    fragmentSource
                );

            GLuint program =
                LinkProgram(
                    vertexShader,
                    fragmentShader
                );

            glDeleteShader(
                vertexShader
            );

            glDeleteShader(
                fragmentShader
            );

            return program;
        }
        catch (...)
        {
            if (vertexShader)
            {
                glDeleteShader(
                    vertexShader
                );
            }

            if (fragmentShader)
            {
                glDeleteShader(
                    fragmentShader
                );
            }

            throw;
        }
    }
}

Application::Application()
{
    // Pin::node stores a Node*. Reserve enough space so ordinary hackathon use
    // will not invalidate those pointers.
    nodes.reserve(256);
    links.reserve(512);

    InitGlfw();
    CreateViewportFramebuffer();

    glGenVertexArrays(
        1,
        &triangleVAO
    );

    InitImGui();

    ed::SetCurrentEditor(
        m_Context
    );

    inputNode =
        AddNode(
            NodeType::Input,
            "Input",
            {},
            {
                { "Point", PinData::Vec3 }
            }
        );

    outputNode =
        AddNode(
            NodeType::Output,
            "Output",
            {
                { "Distance", PinData::Float },
                { "Material ID", PinData::Float }
            },
            {}
        );

    // Safe default when nothing is connected:
    // the scene is effectively infinitely far away.
    outputNode->inputs[0].floatValue =
        1000.0f;

    outputNode->inputs[1].floatValue =
        0.0f;

    ed::SetNodePosition(
        inputNode->uniqueId,
        ImVec2(-400.0f, 0.0f)
    );

    ed::SetNodePosition(
        outputNode->uniqueId,
        ImVec2(400.0f, 0.0f)
    );

    ed::SetCurrentEditor(
        nullptr
    );

    RecompileGraphShader();
}

Application::~Application()
{
    if (triangleProgram)
    {
        glDeleteProgram(
            triangleProgram
        );
    }

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
    while (
        !glfwWindowShouldClose(window)
        )
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
        glCheckFramebufferStatus(
            GL_FRAMEBUFFER
        ) != GL_FRAMEBUFFER_COMPLETE
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
    if (!glfwInit())
    {
        throw std::runtime_error(
            "Failed to initialize GLFW"
        );
    }

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
            nullptr,
            nullptr
        );

    if (!window)
    {
        glfwTerminate();

        throw std::runtime_error(
            "Failed to create window"
        );
    }

    glfwMakeContextCurrent(
        window
    );

    if (
        !gladLoadGLLoader(
            (GLADloadproc)
            glfwGetProcAddress
        )
        )
    {
        glfwTerminate();

        throw std::runtime_error(
            "Failed to initialize GLAD"
        );
    }

    glfwSwapInterval(1);

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

    // Small debounce:
    // topology changes feel immediate, but dragging a float does not trigger
    // dozens of shader compiler invocations per second.
    if (
        graphDirty &&
        glfwGetTime() - graphDirtySince >
        0.06
        )
    {
        RecompileGraphShader();
    }

    if (
        viewportWidth > 0 &&
        viewportHeight > 0 &&
        triangleProgram != 0
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
            (ImTextureID)
            (intptr_t)
            texture,
            size,
            ImVec2(0, 1),
            ImVec2(1, 0)
        );
    }

    ImGui::End();
}

void Application::DrawNodeEditor()
{
    bool visible =
        ImGui::Begin(
            "Node Editor"
        );

    if (!visible)
    {
        ImGui::End();
        return;
    }

    ed::SetCurrentEditor(
        m_Context
    );

    bool goToMain =
        ImGui::Button(
            "Go to Main"
        );

    ImGui::SameLine();

    if (shaderError.empty())
    {
        ImGui::TextUnformatted(
            "Shader: OK"
        );
    }
    else
    {
        ImGui::TextUnformatted(
            "Shader: compile error"
        );

        if (
            ImGui::IsItemHovered()
            )
        {
            ImGui::SetTooltip(
                "%s",
                shaderError.c_str()
            );
        }
    }

    ed::Begin(
        "My Editor",
        ImVec2(
            0.0f,
            0.0f
        )
    );

    for (auto& node : nodes)
    {
        ed::BeginNode(
            node.uniqueId
        );

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

            // Output is intentionally wiring-only.
            // Every other input can fall back to a literal value.
            if (
                node.type != NodeType::Output &&
                !IsInputConnected(input)
                )
            {
                ImGui::SameLine();

                if (
                    DrawPinValueEditor(
                        input
                    )
                    )
                {
                    MarkGraphDirty();
                }
            }
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

    if (goToMain)
    {
        for (auto& node : nodes)
        {
            ed::DeselectNode(
                node.uniqueId
            );
        }

        if (inputNode)
        {
            ed::SelectNode(
                inputNode->uniqueId,
                true
            );
        }

        if (outputNode)
        {
            ed::SelectNode(
                outputNode->uniqueId,
                true
            );
        }

        ed::NavigateToSelection();
    }

    for (auto& link : links)
    {
        ed::Link(
            link.uniqueId,
            link.outputPin,
            link.inputPin
        );
    }

    if (
        ed::BeginCreate()
        )
    {
        ed::PinId pinAId = 0;
        ed::PinId pinBId = 0;

        if (
            ed::QueryNewLink(
                &pinAId,
                &pinBId
            )
            )
        {
            Pin* pinA =
                FindPin(
                    pinAId
                );

            Pin* pinB =
                FindPin(
                    pinBId
                );

            Pin* outputPin =
                nullptr;

            Pin* inputPin =
                nullptr;

            if (
                pinA &&
                pinB
                )
            {
                if (
                    pinA->type ==
                    PinType::Output &&
                    pinB->type ==
                    PinType::Input
                    )
                {
                    outputPin =
                        pinA;

                    inputPin =
                        pinB;
                }
                else if (
                    pinA->type ==
                    PinType::Input &&
                    pinB->type ==
                    PinType::Output
                    )
                {
                    outputPin =
                        pinB;

                    inputPin =
                        pinA;
                }

                if (
                    outputPin &&
                    inputPin &&
                    CanCreateLink(
                        outputPin,
                        inputPin
                    )
                    )
                {
                    if (
                        ed::AcceptNewItem()
                        )
                    {
                        // One incoming edge per input pin.
                        links.erase(
                            std::remove_if(
                                links.begin(),
                                links.end(),
                                [inputPin](
                                    const Link& link
                                    )
                                {
                                    return
                                        link.inputPin ==
                                        inputPin->uniqueId;
                                }
                            ),
                            links.end()
                        );

                        Link link;

                        link.uniqueId =
                            ed::LinkId(
                                GetNextNodeEditorId()
                            );

                        link.outputPin =
                            outputPin->uniqueId;

                        link.inputPin =
                            inputPin->uniqueId;

                        links.push_back(
                            link
                        );

                        MarkGraphDirty();
                    }
                }
                else
                {
                    ed::RejectNewItem();
                }
            }
        }
    }

    // Important with imgui-node-editor:
    // EndCreate must be paired every frame.
    ed::EndCreate();

    if (
        ed::BeginDelete()
        )
    {
        ed::LinkId deletedLinkId = 0;

        while (
            ed::QueryDeletedLink(
                &deletedLinkId
            )
            )
        {
            if (
                ed::AcceptDeletedItem()
                )
            {
                links.erase(
                    std::remove_if(
                        links.begin(),
                        links.end(),
                        [deletedLinkId](
                            const Link& link
                            )
                        {
                            return
                                link.uniqueId ==
                                deletedLinkId;
                        }
                    ),
                    links.end()
                );

                MarkGraphDirty();
            }
        }
    }

    ed::EndDelete();

    static ImVec2 createNodePosition;

    ed::Suspend();

    if (
        ed::ShowBackgroundContextMenu()
        )
    {
        createNodePosition =
            ed::ScreenToCanvas(
                ImGui::GetMousePos()
            );

        ImGui::OpenPopup(
            "Create Node"
        );
    }

    ed::Resume();

    ed::Suspend();

    if (
        ImGui::BeginPopup(
            "Create Node"
        )
        )
    {
        Node* node = nullptr;

        ImGui::TextUnformatted(
            "Shapes"
        );

        if (
            ImGui::MenuItem(
                "Sphere"
            )
            )
        {
            node =
                SpawnNode(
                    NodeType::Sphere
                );
        }

        if (
            ImGui::MenuItem(
                "Box"
            )
            )
        {
            node =
                SpawnNode(
                    NodeType::Box
                );
        }

        if (
            ImGui::MenuItem(
                "Terrain SDF"
            )
            )
        {
            node =
                SpawnNode(
                    NodeType::TerrainSDF
                );
        }

        ImGui::Separator();
        ImGui::TextUnformatted(
            "Space / coordinates"
        );

        if (
            ImGui::MenuItem(
                "Translate"
            )
            )
        {
            node =
                SpawnNode(
                    NodeType::Translate
                );
        }

        if (
            ImGui::MenuItem(
                "Rotate XZ"
            )
            )
        {
            node =
                SpawnNode(
                    NodeType::RotateXZ
                );
        }

        if (
            ImGui::MenuItem(
                "Repeat XZ"
            )
            )
        {
            node =
                SpawnNode(
                    NodeType::RepeatXZ
                );
        }

        if (
            ImGui::MenuItem(
                "Get XZ"
            )
            )
        {
            node =
                SpawnNode(
                    NodeType::GetXZ
                );
        }

        if (
            ImGui::MenuItem(
                "Terrain Height"
            )
            )
        {
            node =
                SpawnNode(
                    NodeType::TerrainHeight
                );
        }

        if (
            ImGui::MenuItem(
                "Offset Y"
            )
            )
        {
            node =
                SpawnNode(
                    NodeType::OffsetY
                );
        }

        if (
            ImGui::MenuItem(
                "Scale Y"
            )
            )
        {
            node =
                SpawnNode(
                    NodeType::ScaleY
                );
        }

        ImGui::Separator();
        ImGui::TextUnformatted(
            "SDF combine"
        );

        if (
            ImGui::MenuItem(
                "Union"
            )
            )
        {
            node =
                SpawnNode(
                    NodeType::Union
                );
        }

        if (
            ImGui::MenuItem(
                "Smooth Union"
            )
            )
        {
            node =
                SpawnNode(
                    NodeType::SmoothUnion
                );
        }

        if (
            ImGui::MenuItem(
                "Intersection"
            )
            )
        {
            node =
                SpawnNode(
                    NodeType::Intersection
                );
        }

        if (
            ImGui::MenuItem(
                "Difference"
            )
            )
        {
            node =
                SpawnNode(
                    NodeType::Difference
                );
        }

        ImGui::Separator();
        ImGui::TextUnformatted(
            "Float math"
        );

        if (
            ImGui::MenuItem(
                "Add"
            )
            )
        {
            node =
                SpawnNode(
                    NodeType::Add
                );
        }

        if (
            ImGui::MenuItem(
                "Subtract"
            )
            )
        {
            node =
                SpawnNode(
                    NodeType::Subtract
                );
        }

        if (
            ImGui::MenuItem(
                "Multiply"
            )
            )
        {
            node =
                SpawnNode(
                    NodeType::Multiply
                );
        }

        if (
            ImGui::MenuItem(
                "Divide"
            )
            )
        {
            node =
                SpawnNode(
                    NodeType::Divide
                );
        }

        if (node)
        {
            ed::SetNodePosition(
                node->uniqueId,
                createNodePosition
            );

            MarkGraphDirty();
        }

        ImGui::EndPopup();
    }

    ed::Resume();

    ed::End();

    ed::SetCurrentEditor(
        nullptr
    );

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
    NodeType type,
    const std::string& name,
    const std::vector<
    std::pair<
    std::string,
    PinData
    >
    >& inputs,
    const std::vector<
    std::pair<
    std::string,
    PinData
    >
    >& outputs
)
{
    nodes.push_back(
        Node{}
    );

    Node& node =
        nodes.back();

    node.uniqueId =
        ed::NodeId(
            GetNextNodeEditorId()
        );

    node.type =
        type;

    node.name =
        name;

    for (
        const auto& input :
        inputs
        )
    {
        Pin pin;

        pin.uniqueId =
            ed::PinId(
                GetNextNodeEditorId()
            );

        pin.node =
            &node;

        pin.name =
            input.first;

        pin.type =
            PinType::Input;

        pin.data =
            input.second;

        node.inputs.push_back(
            pin
        );
    }

    for (
        const auto& output :
        outputs
        )
    {
        Pin pin;

        pin.uniqueId =
            ed::PinId(
                GetNextNodeEditorId()
            );

        pin.node =
            &node;

        pin.name =
            output.first;

        pin.type =
            PinType::Output;

        pin.data =
            output.second;

        node.outputs.push_back(
            pin
        );
    }

    return &node;
}

Node* Application::SpawnNode(
    NodeType type
)
{
    Node* node = nullptr;

    switch (type)
    {
    case NodeType::Sphere:
    {
        node =
            AddNode(
                type,
                "Sphere",
                {
                    { "Point", PinData::Vec3 },
                    { "Radius", PinData::Float },
                    { "Material ID", PinData::Float }
                },
                    {
                        { "Distance", PinData::Float },
                        { "Material ID", PinData::Float }
                    }
            );

        node->inputs[1].floatValue =
            0.5f;

        node->inputs[2].floatValue =
            1.0f;

        break;
    }

    case NodeType::Box:
    {
        node =
            AddNode(
                type,
                "Box",
                {
                    { "Point", PinData::Vec3 },
                    { "Half Size", PinData::Vec3 },
                    { "Material ID", PinData::Float }
                },
                    {
                        { "Distance", PinData::Float },
                        { "Material ID", PinData::Float }
                    }
            );

        node->inputs[1].vec3Value[0] =
            0.5f;

        node->inputs[1].vec3Value[1] =
            0.5f;

        node->inputs[1].vec3Value[2] =
            0.5f;

        node->inputs[2].floatValue =
            2.0f;

        break;
    }

    case NodeType::TerrainSDF:
    {
        node =
            AddNode(
                type,
                "Terrain SDF",
                {
                    { "Point", PinData::Vec3 },
                    { "Frequency", PinData::Float },
                    { "Height", PinData::Float },
                    { "Quality", PinData::Float },
                    { "Material ID", PinData::Float }
                },
                    {
                        { "Distance", PinData::Float },
                        { "Material ID", PinData::Float }
                    }
            );

        node->inputs[1].floatValue =
            1.0f;

        node->inputs[2].floatValue =
            1.0f;

        node->inputs[3].floatValue =
            8.0f;

        node->inputs[4].floatValue =
            0.0f;

        break;
    }

    case NodeType::Translate:
    {
        node =
            AddNode(
                type,
                "Translate",
                {
                    { "Point", PinData::Vec3 },
                    { "Offset", PinData::Vec3 }
                },
                    {
                        { "Point", PinData::Vec3 }
                    }
            );

        break;
    }

    case NodeType::RotateXZ:
    {
        node =
            AddNode(
                type,
                "Rotate XZ",
                {
                    { "Point", PinData::Vec3 },
                    { "Angle", PinData::Float }
                },
                    {
                        { "Point", PinData::Vec3 }
                    }
            );

        break;
    }

    case NodeType::RepeatXZ:
    {
        node =
            AddNode(
                type,
                "Repeat XZ",
                {
                    { "Point", PinData::Vec3 },
                    { "Period", PinData::Float }
                },
                    {
                        { "Local Point", PinData::Vec3 },
                        { "Cell Center", PinData::Vec2 }
                    }
            );

        node->inputs[1].floatValue =
            0.5f;

        break;
    }

    case NodeType::GetXZ:
    {
        node =
            AddNode(
                type,
                "Get XZ",
                {
                    { "Point", PinData::Vec3 }
                },
                    {
                        { "XZ", PinData::Vec2 }
                    }
            );

        break;
    }

    case NodeType::TerrainHeight:
    {
        node =
            AddNode(
                type,
                "Terrain Height",
                {
                    { "XZ", PinData::Vec2 },
                    { "Frequency", PinData::Float },
                    { "Height", PinData::Float },
                    { "Quality", PinData::Float }
                },
                    {
                        { "Height", PinData::Float }
                    }
            );

        node->inputs[1].floatValue =
            1.0f;

        node->inputs[2].floatValue =
            1.0f;

        node->inputs[3].floatValue =
            8.0f;

        break;
    }

    case NodeType::OffsetY:
    {
        node =
            AddNode(
                type,
                "Offset Y",
                {
                    { "Point", PinData::Vec3 },
                    { "Offset", PinData::Float }
                },
                    {
                        { "Point", PinData::Vec3 }
                    }
            );

        break;
    }

    case NodeType::ScaleY:
    {
        node =
            AddNode(
                type,
                "Scale Y",
                {
                    { "Point", PinData::Vec3 },
                    { "Divisor", PinData::Float }
                },
                    {
                        { "Point", PinData::Vec3 }
                    }
            );

        node->inputs[1].floatValue =
            3.0f;

        break;
    }

    case NodeType::Union:
    case NodeType::Intersection:
    case NodeType::Difference:
    {
        const char* name =
            type == NodeType::Union
            ? "Union"
            : (
                type ==
                NodeType::Intersection
                ? "Intersection"
                : "Difference"
                );

        node =
            AddNode(
                type,
                name,
                {
                    { "A Distance", PinData::Float },
                    { "A Material", PinData::Float },
                    { "B Distance", PinData::Float },
                    { "B Material", PinData::Float }
                },
                    {
                        { "Distance", PinData::Float },
                        { "Material ID", PinData::Float }
                    }
            );

        node->inputs[0].floatValue =
            1000.0f;

        node->inputs[2].floatValue =
            1000.0f;

        break;
    }

    case NodeType::SmoothUnion:
    {
        node =
            AddNode(
                type,
                "Smooth Union",
                {
                    { "A Distance", PinData::Float },
                    { "A Material", PinData::Float },
                    { "B Distance", PinData::Float },
                    { "B Material", PinData::Float },
                    { "Smoothness", PinData::Float }
                },
                    {
                        { "Distance", PinData::Float },
                        { "Material ID", PinData::Float }
                    }
            );

        node->inputs[0].floatValue =
            1000.0f;

        node->inputs[2].floatValue =
            1000.0f;

        node->inputs[4].floatValue =
            0.2f;

        break;
    }

    case NodeType::Add:
    case NodeType::Subtract:
    case NodeType::Multiply:
    case NodeType::Divide:
    {
        const char* name = "Add";

        if (
            type ==
            NodeType::Subtract
            )
        {
            name =
                "Subtract";
        }
        else if (
            type ==
            NodeType::Multiply
            )
        {
            name =
                "Multiply";
        }
        else if (
            type ==
            NodeType::Divide
            )
        {
            name =
                "Divide";
        }

        node =
            AddNode(
                type,
                name,
                {
                    { "A", PinData::Float },
                    { "B", PinData::Float }
                },
                    {
                        { "Result", PinData::Float }
                    }
            );

        if (
            type ==
            NodeType::Multiply
            )
        {
            node->inputs[0].floatValue =
                1.0f;

            node->inputs[1].floatValue =
                1.0f;
        }

        if (
            type ==
            NodeType::Divide
            )
        {
            node->inputs[1].floatValue =
                1.0f;
        }

        break;
    }

    default:
    {
        break;
    }
    }

    return node;
}

Pin* Application::FindPin(
    ed::PinId id
)
{
    for (auto& node : nodes)
    {
        for (
            auto& pin :
            node.inputs
            )
        {
            if (
                pin.uniqueId ==
                id
                )
            {
                return &pin;
            }
        }

        for (
            auto& pin :
            node.outputs
            )
        {
            if (
                pin.uniqueId ==
                id
                )
            {
                return &pin;
            }
        }
    }

    return nullptr;
}

Pin* Application::FindConnectedOutput(
    ed::PinId inputPinId
)
{
    for (auto& link : links)
    {
        if (
            link.inputPin ==
            inputPinId
            )
        {
            return FindPin(
                link.outputPin
            );
        }
    }

    return nullptr;
}

bool Application::IsInputConnected(
    const Pin& pin
) const
{
    for (
        const auto& link :
        links
        )
    {
        if (
            link.inputPin ==
            pin.uniqueId
            )
        {
            return true;
        }
    }

    return false;
}

bool Application::CanCreateLink(
    Pin* a,
    Pin* b
)
{
    if (
        !a ||
        !b
        )
    {
        return false;
    }

    if (
        a ==
        b
        )
    {
        return false;
    }

    if (
        a->node ==
        b->node
        )
    {
        return false;
    }

    if (
        a->type !=
        PinType::Output ||
        b->type !=
        PinType::Input
        )
    {
        return false;
    }

    if (
        a->data !=
        b->data
        )
    {
        return false;
    }

    return true;
}

bool Application::DrawPinValueEditor(
    Pin& pin
)
{
    ImGui::PushID(
        pin.uniqueId.AsPointer()
    );

    ImGui::PushItemWidth(
        pin.data == PinData::Float
        ? 90.0f
        : 150.0f
    );

    bool changed = false;

    switch (pin.data)
    {
    case PinData::Float:
    {
        changed =
            ImGui::DragFloat(
                "##value",
                &pin.floatValue,
                0.01f
            );

        break;
    }

    case PinData::Vec2:
    {
        changed =
            ImGui::DragFloat2(
                "##value",
                pin.vec2Value,
                0.01f
            );

        break;
    }

    case PinData::Vec3:
    {
        changed =
            ImGui::DragFloat3(
                "##value",
                pin.vec3Value,
                0.01f
            );

        break;
    }

    case PinData::Vec4:
    {
        changed =
            ImGui::DragFloat4(
                "##value",
                pin.vec4Value,
                0.01f
            );

        break;
    }
    }

    ImGui::PopItemWidth();
    ImGui::PopID();

    return changed;
}

void Application::MarkGraphDirty()
{
    graphDirty = true;

    graphDirtySince =
        glfwGetTime();
}

void Application::RecompileGraphShader()
{
    graphDirty = false;

    try
    {
        std::string generatedSource =
            BuildGeneratedFragmentShader();

        WriteFile(
            generatedFragmentPath.c_str(),
            generatedSource
        );

        std::string vertexSource =
            ReadFile(
                vertexShaderPath.c_str()
            );

        GLuint newProgram =
            CreateProgramFromSources(
                vertexSource,
                generatedSource
            );

        if (triangleProgram)
        {
            glDeleteProgram(
                triangleProgram
            );
        }

        triangleProgram =
            newProgram;

        shaderError.clear();
    }
    catch (
        const std::exception& e
        )
    {
        // Keep the last valid shader alive.
        shaderError =
            e.what();
    }
}

std::string Application::BuildGeneratedFragmentShader()
{
    std::string source =
        ReadFile(
            fragmentTemplatePath.c_str()
        );

    const std::string marker =
        "//__AXIO_SCENE_FUNCTION__";

    size_t position =
        source.find(marker);

    if (
        position ==
        std::string::npos
        )
    {
        throw std::runtime_error(
            "frag_template.glsl is missing //__AXIO_SCENE_FUNCTION__"
        );
    }

    source.replace(
        position,
        marker.size(),
        BuildSceneFunction()
    );

    return source;
}

std::string Application::BuildSceneFunction()
{
    if (
        !outputNode ||
        outputNode->inputs.size() < 2
        )
    {
        throw std::runtime_error(
            "Output node is missing"
        );
    }

    GraphCompileContext context;

    std::string distance =
        CompileInputPin(
            outputNode->inputs[0],
            context
        );

    std::string material =
        CompileInputPin(
            outputNode->inputs[1],
            context
        );

    std::ostringstream out;

    out
        << "SceneSample sceneSDF(vec3 p)\n"
        << "{\n"
        << context.code.str()
        << "    return SceneSample("
        << distance
        << ", "
        << material
        << ");\n"
        << "}\n";

    return out.str();
}

std::string Application::CompileInputPin(
    const Pin& input,
    GraphCompileContext& context
)
{
    Pin* connectedOutput =
        FindConnectedOutput(
            input.uniqueId
        );

    if (connectedOutput)
    {
        return CompileOutputPin(
            connectedOutput,
            context
        );
    }

    return LiteralForPin(
        input
    );
}

std::string Application::CompileOutputPin(
    Pin* output,
    GraphCompileContext& context
)
{
    if (!output)
    {
        throw std::runtime_error(
            "Compiler received a null output pin"
        );
    }

    if (
        output->node &&
        output->node->type ==
        NodeType::Input
        )
    {
        return "p";
    }

    auto existing =
        context.outputExpressions.find(
            output
        );

    if (
        existing !=
        context.outputExpressions.end()
        )
    {
        return existing->second;
    }

    CompileNode(
        output->node,
        context
    );

    existing =
        context.outputExpressions.find(
            output
        );

    if (
        existing ==
        context.outputExpressions.end()
        )
    {
        throw std::runtime_error(
            "Node compiler did not produce the requested output"
        );
    }

    return existing->second;
}

void Application::CompileNode(
    Node* node,
    GraphCompileContext& context
)
{
    if (!node)
    {
        throw std::runtime_error(
            "Compiler reached a null node"
        );
    }

    if (
        context.emitted.count(
            node
        ) != 0
        )
    {
        return;
    }

    if (
        context.visiting.count(
            node
        ) != 0
        )
    {
        throw std::runtime_error(
            "Cycle detected in node graph"
        );
    }

    context.visiting.insert(
        node
    );

    int index = 0;

    auto indexIt =
        context.nodeIndices.find(
            node
        );

    if (
        indexIt ==
        context.nodeIndices.end()
        )
    {
        index =
            context.nextNodeIndex++;

        context.nodeIndices[node] =
            index;
    }
    else
    {
        index =
            indexIt->second;
    }

    const std::string base =
        "n" +
        std::to_string(index);

    auto input =
        [
            this,
            &context,
            node
        ](size_t i)
        {
            if (
                i >=
                node->inputs.size()
                )
            {
                throw std::runtime_error(
                    "Node input index out of range"
                );
            }

            return CompileInputPin(
                node->inputs[i],
                context
            );
        };

    auto bindOutput =
        [
            &context,
            node
        ](
            size_t i,
            const std::string& expression
            )
        {
            if (
                i >=
                node->outputs.size()
                )
            {
                throw std::runtime_error(
                    "Node output index out of range"
                );
            }

            context.outputExpressions[
                &node->outputs[i]
            ] =
                expression;
        };

    switch (node->type)
    {
    case NodeType::Sphere:
    {
        const std::string point =
            input(0);

        const std::string radius =
            input(1);

        const std::string material =
            input(2);

        const std::string distanceVar =
            base + "_distance";

        const std::string materialVar =
            base + "_material";

        context.code
            << "    float "
            << distanceVar
            << " = sdSphere("
            << point
            << ", "
            << radius
            << ");\n";

        context.code
            << "    float "
            << materialVar
            << " = "
            << material
            << ";\n";

        bindOutput(
            0,
            distanceVar
        );

        bindOutput(
            1,
            materialVar
        );

        break;
    }

    case NodeType::Box:
    {
        const std::string point =
            input(0);

        const std::string halfSize =
            input(1);

        const std::string material =
            input(2);

        const std::string distanceVar =
            base + "_distance";

        const std::string materialVar =
            base + "_material";

        context.code
            << "    float "
            << distanceVar
            << " = sdBox("
            << point
            << ", "
            << halfSize
            << ");\n";

        context.code
            << "    float "
            << materialVar
            << " = "
            << material
            << ";\n";

        bindOutput(
            0,
            distanceVar
        );

        bindOutput(
            1,
            materialVar
        );

        break;
    }

    case NodeType::TerrainSDF:
    {
        const std::string point =
            input(0);

        const std::string frequency =
            input(1);

        const std::string height =
            input(2);

        const std::string quality =
            input(3);

        const std::string material =
            input(4);

        const std::string distanceVar =
            base + "_distance";

        const std::string materialVar =
            base + "_material";

        context.code
            << "    float "
            << distanceVar
            << " = terrainSDFGraph("
            << point
            << ", "
            << frequency
            << ", "
            << height
            << ", int(clamp("
            << quality
            << ", 1.0, 16.0))"
            << ");\n";

        context.code
            << "    float "
            << materialVar
            << " = "
            << material
            << ";\n";

        bindOutput(
            0,
            distanceVar
        );

        bindOutput(
            1,
            materialVar
        );

        break;
    }

    case NodeType::Translate:
    {
        const std::string point =
            input(0);

        const std::string offset =
            input(1);

        const std::string valueVar =
            base + "_point";

        context.code
            << "    vec3 "
            << valueVar
            << " = "
            << point
            << " - "
            << offset
            << ";\n";

        bindOutput(
            0,
            valueVar
        );

        break;
    }

    case NodeType::RotateXZ:
    {
        const std::string point =
            input(0);

        const std::string angle =
            input(1);

        const std::string valueVar =
            base + "_point";

        context.code
            << "    vec3 "
            << valueVar
            << " = "
            << point
            << ";\n";

        context.code
            << "    "
            << valueVar
            << ".xz = rotate("
            << valueVar
            << ".xz, "
            << angle
            << ");\n";

        bindOutput(
            0,
            valueVar
        );

        break;
    }

    case NodeType::RepeatXZ:
    {
        const std::string point =
            input(0);

        const std::string period =
            input(1);

        const std::string periodVar =
            base + "_period";

        const std::string localVar =
            base + "_local";

        const std::string centerVar =
            base + "_center";

        context.code
            << "    float "
            << periodVar
            << " = max(abs("
            << period
            << "), 0.0001);\n";

        context.code
            << "    vec3 "
            << localVar
            << " = "
            << point
            << ";\n";

        context.code
            << "    "
            << localVar
            << ".xz = tmod("
            << localVar
            << ".xz, vec2("
            << periodVar
            << "));\n";

        context.code
            << "    vec2 "
            << centerVar
            << " = "
            << point
            << ".xz - "
            << localVar
            << ".xz;\n";

        bindOutput(
            0,
            localVar
        );

        bindOutput(
            1,
            centerVar
        );

        break;
    }

    case NodeType::GetXZ:
    {
        const std::string point =
            input(0);

        const std::string valueVar =
            base + "_xz";

        context.code
            << "    vec2 "
            << valueVar
            << " = "
            << point
            << ".xz;\n";

        bindOutput(
            0,
            valueVar
        );

        break;
    }

    case NodeType::TerrainHeight:
    {
        const std::string xz =
            input(0);

        const std::string frequency =
            input(1);

        const std::string height =
            input(2);

        const std::string quality =
            input(3);

        const std::string valueVar =
            base + "_height";

        context.code
            << "    float "
            << valueVar
            << " = graphTerrain("
            << xz
            << " * "
            << frequency
            << ", int(clamp("
            << quality
            << ", 1.0, 16.0))) * "
            << height
            << ";\n";

        bindOutput(
            0,
            valueVar
        );

        break;
    }

    case NodeType::OffsetY:
    {
        const std::string point =
            input(0);

        const std::string offset =
            input(1);

        const std::string valueVar =
            base + "_point";

        context.code
            << "    vec3 "
            << valueVar
            << " = "
            << point
            << ";\n";

        context.code
            << "    "
            << valueVar
            << ".y -= "
            << offset
            << ";\n";

        bindOutput(
            0,
            valueVar
        );

        break;
    }

    case NodeType::ScaleY:
    {
        const std::string point =
            input(0);

        const std::string divisor =
            input(1);

        const std::string divisorVar =
            base + "_divisor";

        const std::string valueVar =
            base + "_point";

        context.code
            << "    float "
            << divisorVar
            << " = max(abs("
            << divisor
            << "), 0.0001);\n";

        context.code
            << "    vec3 "
            << valueVar
            << " = "
            << point
            << ";\n";

        context.code
            << "    "
            << valueVar
            << ".y /= "
            << divisorVar
            << ";\n";

        bindOutput(
            0,
            valueVar
        );

        break;
    }

    case NodeType::Union:
    case NodeType::Intersection:
    case NodeType::Difference:
    case NodeType::SmoothUnion:
    {
        const std::string da =
            input(0);

        const std::string ma =
            input(1);

        const std::string db =
            input(2);

        const std::string mb =
            input(3);

        const std::string daVar =
            base + "_da";

        const std::string dbVar =
            base + "_db";

        const std::string distanceVar =
            base + "_distance";

        const std::string materialVar =
            base + "_material";

        context.code
            << "    float "
            << daVar
            << " = "
            << da
            << ";\n";

        context.code
            << "    float "
            << dbVar
            << " = "
            << db
            << ";\n";

        if (
            node->type ==
            NodeType::Union
            )
        {
            context.code
                << "    float "
                << distanceVar
                << " = min("
                << daVar
                << ", "
                << dbVar
                << ");\n";

            context.code
                << "    float "
                << materialVar
                << " = ("
                << daVar
                << " < "
                << dbVar
                << ") ? "
                << ma
                << " : "
                << mb
                << ";\n";
        }
        else if (
            node->type ==
            NodeType::Intersection
            )
        {
            context.code
                << "    float "
                << distanceVar
                << " = max("
                << daVar
                << ", "
                << dbVar
                << ");\n";

            context.code
                << "    float "
                << materialVar
                << " = ("
                << daVar
                << " > "
                << dbVar
                << ") ? "
                << ma
                << " : "
                << mb
                << ";\n";
        }
        else if (
            node->type ==
            NodeType::Difference
            )
        {
            context.code
                << "    float "
                << distanceVar
                << " = max("
                << daVar
                << ", -("
                << dbVar
                << "));\n";

            context.code
                << "    float "
                << materialVar
                << " = "
                << ma
                << ";\n";
        }
        else
        {
            const std::string k =
                input(4);

            const std::string kVar =
                base + "_k";

            const std::string hVar =
                base + "_h";

            context.code
                << "    float "
                << kVar
                << " = max(abs("
                << k
                << "), 0.0001);\n";

            context.code
                << "    float "
                << hVar
                << " = clamp(0.5 + 0.5 * ("
                << dbVar
                << " - "
                << daVar
                << ") / "
                << kVar
                << ", 0.0, 1.0);\n";

            context.code
                << "    float "
                << distanceVar
                << " = mix("
                << dbVar
                << ", "
                << daVar
                << ", "
                << hVar
                << ") - "
                << kVar
                << " * "
                << hVar
                << " * (1.0 - "
                << hVar
                << ");\n";

            context.code
                << "    float "
                << materialVar
                << " = ("
                << daVar
                << " < "
                << dbVar
                << ") ? "
                << ma
                << " : "
                << mb
                << ";\n";
        }

        bindOutput(
            0,
            distanceVar
        );

        bindOutput(
            1,
            materialVar
        );

        break;
    }

    case NodeType::Add:
    case NodeType::Subtract:
    case NodeType::Multiply:
    case NodeType::Divide:
    {
        const std::string a =
            input(0);

        const std::string b =
            input(1);

        const std::string resultVar =
            base + "_result";

        context.code
            << "    float "
            << resultVar
            << " = ";

        if (
            node->type ==
            NodeType::Add
            )
        {
            context.code
                << "("
                << a
                << ") + ("
                << b
                << ")";
        }
        else if (
            node->type ==
            NodeType::Subtract
            )
        {
            context.code
                << "("
                << a
                << ") - ("
                << b
                << ")";
        }
        else if (
            node->type ==
            NodeType::Multiply
            )
        {
            context.code
                << "("
                << a
                << ") * ("
                << b
                << ")";
        }
        else
        {
            context.code
                << "("
                << a
                << ") / max(abs("
                << b
                << "), 0.0001)";
        }

        context.code
            << ";\n";

        bindOutput(
            0,
            resultVar
        );

        break;
    }

    case NodeType::Input:
    case NodeType::Output:
    {
        break;
    }
    }

    context.visiting.erase(
        node
    );

    context.emitted.insert(
        node
    );
}

std::string Application::LiteralForPin(
    const Pin& pin
) const
{
    switch (pin.data)
    {
    case PinData::Float:
    {
        return GlslFloat(
            pin.floatValue
        );
    }

    case PinData::Vec2:
    {
        return
            "vec2(" +
            GlslFloat(
                pin.vec2Value[0]
            ) +
            ", " +
            GlslFloat(
                pin.vec2Value[1]
            ) +
            ")";
    }

    case PinData::Vec3:
    {
        return
            "vec3(" +
            GlslFloat(
                pin.vec3Value[0]
            ) +
            ", " +
            GlslFloat(
                pin.vec3Value[1]
            ) +
            ", " +
            GlslFloat(
                pin.vec3Value[2]
            ) +
            ")";
    }

    case PinData::Vec4:
    {
        return
            "vec4(" +
            GlslFloat(
                pin.vec4Value[0]
            ) +
            ", " +
            GlslFloat(
                pin.vec4Value[1]
            ) +
            ", " +
            GlslFloat(
                pin.vec4Value[2]
            ) +
            ", " +
            GlslFloat(
                pin.vec4Value[3]
            ) +
            ")";
    }
    }

    return "0.0";
}

std::string Application::GlslFloat(
    float value
) const
{
    if (
        !std::isfinite(value)
        )
    {
        value = 0.0f;
    }

    std::ostringstream out;

    out
        << std::fixed
        << std::setprecision(6)
        << value;

    return out.str();
}
