#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <vector>


// The kinds of public fields of a script class the editor shows and the scene saves (docs/engine/Game_Engine_Runtime_Guide.html,
// 4.5): C#'s bool, int, float, double, string, Vector2, Vector3, Vector4, Entity (or a script class), enums and Prefab
// (6.2). The values are those of ScriptValue.Type (ScriptApi.h) and of FieldKind in ScriptCore/ScriptFields.cs.
enum class ScriptFieldType : int32_t { Bool = 0, Int, Float, Double, String, Vector2, Vector3, Vector4, Entity, Enum, Prefab };

// A field's value as the engine keeps it: in the ScriptComponent (saved in the scene), and as a class's default
struct ScriptFieldValue
{
	ScriptFieldType Type = ScriptFieldType::Float;
	int64_t Integer = 0;                // Bool (0 or 1), Int
	double Number = 0.0;                // Float, Double
	glm::vec4 Vector = glm::vec4(0.0f); // Vector2, Vector3, Vector4 (the first 2 or 3)
	uint64_t Entity = 0;                // Entity: the entity's ID, 0 for none
	std::string Text;                   // String; Enum: the value's name (saved by name: reordering the enum keeps it);
	                                    // Prefab: its file, relative to the project ("Assets/Prefabs/Spark.mprefab"; empty: none)

	bool operator==(const ScriptFieldValue& other) const;
	bool operator!=(const ScriptFieldValue& other) const { return !(*this == other); }

	// "Float", "Vector3"... (the scene file's names) and back; false: no such type
	static const char* GetTypeName(ScriptFieldType type);
	static bool ParseTypeName(const std::string& name, ScriptFieldType& type);

	// The value as another type, when that makes sense (a number as another number: a field changed from float to int
	// keeps its value); false otherwise
	bool ConvertTo(ScriptFieldType type, ScriptFieldValue& out) const;
};

// A public field of a script class, as the game's assembly reports it
struct ScriptFieldInfo
{
	std::string Name;
	ScriptFieldType Type = ScriptFieldType::Float;
	std::string TypeName;                // the C# type: "Single", "Entity", "PlayerController", "Axis"
	std::vector<std::string> EnumNames;  // Enum: its names, in order of declaration
	std::vector<int64_t> EnumValues;     // and their values
	ScriptFieldValue Default;            // the value a new instance has (its initializer)

	// Enum: the value of a name and the name of a value (false: none)
	bool FindEnumValue(const std::string& name, int64_t& value) const;
	bool FindEnumName(int64_t value, std::string& name) const;
};

// A script class of the game: a non-abstract class deriving from Morava.Entity, with its fields
struct ScriptClass
{
	std::string Name; // the full name, with the namespace: "Sample.PlayerController"
	std::vector<ScriptFieldInfo> Fields;

	const ScriptFieldInfo* FindField(const std::string& name) const;
	// The name without the namespace: "PlayerController"
	std::string GetShortName() const;
};
