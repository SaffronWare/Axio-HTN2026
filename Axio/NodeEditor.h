#pragma once

#include <imgui.h>
#include <imgui_node_editor.h>

#include <string>
#include <vector>

namespace ed = ax::NodeEditor;

enum class PinType
{
    Input,
    Output
};

enum class PinData
{
    Float,
    Vec2,
    Vec3,
    Vec4
};

enum class NodeType
{
    Input,
    Output,

    Sphere,
    Box,
    TerrainSDF,

    Translate,
    RotateXZ,
    RepeatXZ,
    GetXZ,
    TerrainHeight,
    OffsetY,
    ScaleY,

    Union,
    SmoothUnion,
    Intersection,
    Difference,

    Add,
    Subtract,
    Multiply,
    Divide
};

struct Node;

struct Pin
{
    ed::PinId uniqueId;
    Node* node = nullptr;

    std::string name;

    PinType type = PinType::Input;
    PinData data = PinData::Float;

    // Literal value used whenever this input pin is not connected.
    float floatValue = 0.0f;
    float vec2Value[2] = { 0.0f, 0.0f };
    float vec3Value[3] = { 0.0f, 0.0f, 0.0f };
    float vec4Value[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
};

struct Node
{
    ed::NodeId uniqueId;

    NodeType type = NodeType::Add;
    std::string name;

    std::vector<Pin> inputs;
    std::vector<Pin> outputs;
};

struct Link
{
    ed::LinkId uniqueId;
    ed::PinId inputPin;
    ed::PinId outputPin;
};
