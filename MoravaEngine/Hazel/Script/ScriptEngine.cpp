#include "ScriptEngine.h"


namespace Hazel
{
	// static MonoDomain* s_MonoDomain = nullptr;
	static std::string s_AssemblyPath;
	static Ref<HazelScene> s_SceneContext;

	// Assembly images
	// MonoImage* s_AppAssemblyImage = nullptr;
	// MonoImage* s_CoreAssemblyImage = nullptr;

	static EntityInstanceMap s_EntityInstanceMap;

	// static MonoMethod* GetMethod(MonoImage* image, const std::string& methodDesc);

	static ScriptModuleFieldMap s_PublicFields;

	PublicField::PublicField(const std::string& name, FieldType type)
		: Name(name), Type(type)
	{
		m_EntityInstance = nullptr;
	}

	void PublicField::SetValue_Internal(void* value) const
	{

	}

	void PublicField::GetValue_Internal(void* outValue) const
	{

	}

	void ScriptEngine::Init(const std::string& assemblyPath)
	{
		Log::GetLogger()->error("ScriptEngine::Init method not implemented yet!");
	}

	void ScriptEngine::Shutdown()
	{
		Log::GetLogger()->error("ScriptEngine::Shutdown method not implemented yet!");
	}

	void ScriptEngine::OnCreateEntity(Entity entity)
	{
		Log::GetLogger()->error("ScriptEngine::OnCreateEntity method not implemented yet!");
	}

	void ScriptEngine::OnUpdateEntity(UUID sceneID, uint32_t entityID, Timestep ts)
	{
		Log::GetLogger()->error("ScriptEngine::OnUpdateEntity method not implemented yet!");
	}

	void ScriptEngine::InitScriptEntity(Entity entity)
	{
		Log::GetLogger()->error("ScriptEngine::InitScriptEntity method not implemented yet!");
	}

	void ScriptEngine::OnInitEntity(ScriptComponent& script, uint32_t entityID, uint32_t sceneID)
	{
		Log::GetLogger()->error("ScriptEngine::OnInitEntity method not implemented yet!");
	}

	const ScriptModuleFieldMap& ScriptEngine::GetFieldMap()
	{
		return s_PublicFields;
	}

	void ScriptEngine::InstantiateEntityClass(Entity entity)
	{
		Log::GetLogger()->error("ScriptEngine::InstantiateEntityClass method not implemented yet!");
	}

	bool ScriptEngine::ModuleExists(std::string moduleName)
	{
		Log::GetLogger()->error("ScriptEngine::ModuleExists method not implemented yet!");
		return true;
	}

	void ScriptEngine::CopyEntityScriptData(UUID dstSceneID, UUID srcSceneID)
	{
		Log::GetLogger()->error("ScriptEngine::CopyEntityScriptData method not implemented yet!");
	}

	EntityInstanceMap& ScriptEngine::GetEntityInstanceMap()
	{
		return s_EntityInstanceMap;
	}

	EntityInstanceData& ScriptEngine::GetEntityInstanceData(UUID sceneID, UUID entityID)
	{
		HZ_CORE_ASSERT(s_EntityInstanceMap.find(sceneID) != s_EntityInstanceMap.end(), "Invalid scene ID!");
		auto& entityIDMap = s_EntityInstanceMap.at(sceneID);
		HZ_CORE_ASSERT(entityIDMap.find(entityID) != entityIDMap.end(), "Invalid entity ID!");
		return entityIDMap.at(entityID);
	}

	const char* FieldTypeToString(FieldType type)
	{
		switch (type)
		{
		case FieldType::Float:       return "Float";
		case FieldType::Int:         return "Int";
		case FieldType::UnsignedInt: return "UnsignedInt";
		case FieldType::String:      return "String";
		case FieldType::Vec2:        return "Vec2";
		case FieldType::Vec3:        return "Vec3";
		case FieldType::Vec4:        return "Vec4";
		}
		return "Unknown";
	}

}
