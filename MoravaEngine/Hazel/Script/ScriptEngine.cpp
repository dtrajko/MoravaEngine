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

}
