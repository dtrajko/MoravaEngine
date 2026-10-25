#include "ScriptField.h"

#include <cmath>
#include <iterator>


static const char* const s_TypeNames[] = { "Bool", "Int", "Float", "Double", "String", "Vector2", "Vector3", "Vector4", "Entity", "Enum", "Prefab" };

bool ScriptFieldValue::operator==(const ScriptFieldValue& other) const
{
	if (Type != other.Type)
	{
		return false;
	}
	switch (Type)
	{
	case ScriptFieldType::Bool:
	case ScriptFieldType::Int:     return Integer == other.Integer;
	case ScriptFieldType::Float:
	case ScriptFieldType::Double:  return Number == other.Number;
	case ScriptFieldType::Vector2: return glm::vec2(Vector) == glm::vec2(other.Vector);
	case ScriptFieldType::Vector3: return glm::vec3(Vector) == glm::vec3(other.Vector);
	case ScriptFieldType::Vector4: return Vector == other.Vector;
	case ScriptFieldType::Entity:  return Entity == other.Entity;
	case ScriptFieldType::String:
	case ScriptFieldType::Enum:
	case ScriptFieldType::Prefab:  return Text == other.Text;
	}
	return false;
}

const char* ScriptFieldValue::GetTypeName(ScriptFieldType type)
{
	const int index = int(type);
	return index >= 0 && index < int(std::size(s_TypeNames)) ? s_TypeNames[index] : "?";
}

bool ScriptFieldValue::ParseTypeName(const std::string& name, ScriptFieldType& type)
{
	for (int i = 0; i < int(std::size(s_TypeNames)); i++)
	{
		if (name == s_TypeNames[i])
		{
			type = ScriptFieldType(i);
			return true;
		}
	}
	return false;
}

static bool IsNumber(ScriptFieldType type)
{
	return type == ScriptFieldType::Int || type == ScriptFieldType::Float || type == ScriptFieldType::Double;
}

static bool IsVector(ScriptFieldType type)
{
	return type == ScriptFieldType::Vector2 || type == ScriptFieldType::Vector3 || type == ScriptFieldType::Vector4;
}

bool ScriptFieldValue::ConvertTo(ScriptFieldType type, ScriptFieldValue& out) const
{
	if (type == Type)
	{
		out = *this;
		return true;
	}
	out = ScriptFieldValue();
	out.Type = type;
	if (IsNumber(Type) && IsNumber(type))
	{
		const double number = Type == ScriptFieldType::Int ? double(Integer) : Number;
		if (type == ScriptFieldType::Int)
		{
			out.Integer = int64_t(std::llround(number));
		}
		else
		{
			out.Number = number;
		}
		return true;
	}
	if (IsVector(Type) && IsVector(type))
	{
		out.Vector = Vector; // the components the smaller one has
		return true;
	}
	return false;
}

bool ScriptFieldInfo::FindEnumValue(const std::string& name, int64_t& value) const
{
	for (size_t i = 0; i < EnumNames.size() && i < EnumValues.size(); i++)
	{
		if (EnumNames[i] == name)
		{
			value = EnumValues[i];
			return true;
		}
	}
	return false;
}

bool ScriptFieldInfo::FindEnumName(int64_t value, std::string& name) const
{
	for (size_t i = 0; i < EnumNames.size() && i < EnumValues.size(); i++)
	{
		if (EnumValues[i] == value)
		{
			name = EnumNames[i];
			return true;
		}
	}
	return false;
}

const ScriptFieldInfo* ScriptClass::FindField(const std::string& name) const
{
	for (const ScriptFieldInfo& field : Fields)
	{
		if (field.Name == name)
		{
			return &field;
		}
	}
	return nullptr;
}

std::string ScriptClass::GetShortName() const
{
	const size_t dot = Name.find_last_of(".+");
	return dot == std::string::npos ? Name : Name.substr(dot + 1);
}
