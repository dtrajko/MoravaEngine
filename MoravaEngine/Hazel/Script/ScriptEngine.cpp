#include "ScriptEngine.h"


namespace Hazel
{

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

	}

	void ScriptEngine::Shutdown()
	{

	}

	void ScriptEngine::OnCreateEntity(Entity entity)
	{

	}

	void ScriptEngine::OnUpdateEntity(uint32_t entityID, Timestep ts)
	{

	}

	void ScriptEngine::InitScriptEntity(Entity entity)
	{
		Log::GetLogger()->error("ScriptEngine::InitScriptEntity method not implemented yet!");
	}

	void ScriptEngine::OnInitEntity(ScriptComponent& script, uint32_t entityID, uint32_t sceneID)
	{

	}

	const ScriptModuleFieldMap& ScriptEngine::GetFieldMap()
	{
		return s_PublicFields;
	}

	void ScriptEngine::InstantiateEntityClass(Entity entity)
	{
		Log::GetLogger()->error("ScriptEngine::InstantiateEntityClass method not implemented yet!");
	}

}
