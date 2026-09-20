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
	Vec4,
	Vec3,
	Vec2
};

struct Node;

struct Pin
{
	ed::PinId uniqueId;

	Node* node = nullptr;

	std::string name;

	PinType type;
	PinData data;
};

struct Node
{
	ed::NodeId uniqueId;

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