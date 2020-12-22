#include "ScriptEngine.h"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/attrdefs.h>

#include <iostream>
#include <chrono>
#include <thread>

#include <Windows.h>
#include <winioctl.h>

#include "ScriptEngineRegistry.h"

#include "../Scene/HazelScene.h"

#include "imgui.h"


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

	PublicField::PublicField(PublicField&& other)
	{
	}

	PublicField::~PublicField()
	{
	}

	void PublicField::CopyStoredValueToRuntime()
	{
	}

	bool PublicField::IsRuntimeAvailable() const
	{
		return false;
	}

	void PublicField::SetStoredValueRaw(void* src)
	{
	}

	uint8_t* PublicField::AllocateBuffer(FieldType type)
	{
		return nullptr;
	}

	void PublicField::SetStoredValue_Internal(void* value) const
	{
	}

	void PublicField::GetStoredValue_Internal(void* outValue) const
	{
	}

	void PublicField::SetRuntimeValue_Internal(void* value) const
	{
	}

	void PublicField::GetRuntimeValue_Internal(void* outValue) const
	{
	}

	// void PublicField::SetValue_Internal(void* value) const
	// {
	// }

	// void PublicField::GetValue_Internal(void* outValue) const
	// {
	// }

	void ScriptEngine::Init(const std::string& assemblyPath)
	{
		Log::GetLogger()->error("ScriptEngine::Init method not implemented yet!");
	}

	void ScriptEngine::Shutdown()
	{
		Log::GetLogger()->error("ScriptEngine::Shutdown method not implemented yet!");
	}

	void ScriptEngine::OnSceneDestruct(UUID sceneID)
	{
	}

	void ScriptEngine::LoadHazelRuntimeAssembly(const std::string& path)
	{
	}

	void ScriptEngine::ReloadAssembly(const std::string& path)
	{
	}

	void ScriptEngine::SetSceneContext(const Ref<HazelScene>& scene)
	{
	}

	const Ref<HazelScene>& ScriptEngine::GetCurrentSceneContext()
	{
		// TODO: insert return statement here

		return Ref<HazelScene>();
	}

	void ScriptEngine::OnCreateEntity(Entity entity)
	{
		Log::GetLogger()->error("ScriptEngine::OnCreateEntity method not implemented yet!");
	}

	void ScriptEngine::OnCreateEntity(UUID sceneID, UUID entityID)
	{
	}

	void ScriptEngine::OnUpdateEntity(UUID sceneID, UUID entityID, Timestep ts)
	{
		Log::GetLogger()->error("ScriptEngine::OnUpdateEntity method not implemented yet!");
	}

	void ScriptEngine::OnCollision2DBegin(Entity entity)
	{
	}

	void ScriptEngine::OnCollision2DBegin(UUID sceneID, UUID entityID)
	{
	}

	void ScriptEngine::OnCollision2DEnd(Entity entity)
	{
	}

	void ScriptEngine::OnCollision2DEnd(UUID sceneID, UUID entityID)
	{
	}

	void ScriptEngine::OnScriptComponentDestroyed(UUID sceneID, UUID entityID)
	{
	}

	bool ScriptEngine::ModuleExists(const std::string& moduleName)
	{
		Log::GetLogger()->error("ScriptEngine::ModuleExists method not implemented yet!");
		return true;
	}

	void ScriptEngine::InitScriptEntity(Entity entity)
	{
		Log::GetLogger()->error("ScriptEngine::InitScriptEntity method not implemented yet!");
	}

	void ScriptEngine::ShutdownScriptEntity(Entity entity, const std::string& moduleName)
	{
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

	void ScriptEngine::CopyEntityScriptData(UUID dst, UUID src)
	{
		HZ_CORE_ASSERT(s_EntityInstanceMap.find(dst) != s_EntityInstanceMap.end());
		HZ_CORE_ASSERT(s_EntityInstanceMap.find(src) != s_EntityInstanceMap.end());

		auto& dstEntityMap = s_EntityInstanceMap.at(dst);
		auto& srcEntityMap = s_EntityInstanceMap.at(src);

		for (auto& [entityID, entityInstanceData] : srcEntityMap)
		{
			for (auto& [moduleName, srcFieldMap] : srcEntityMap[entityID].ModuleFieldMap)
			{
				auto& dstModuleFieldMap = dstEntityMap[entityID].ModuleFieldMap;
				for (auto& [fieldName, field] : srcFieldMap)
				{
					HZ_CORE_ASSERT(dstModuleFieldMap.find(moduleName) != dstModuleFieldMap.end());
					auto& fieldMap = dstModuleFieldMap.at(moduleName);
					HZ_CORE_ASSERT(fieldMap.find(fieldName) != fieldMap.end());
					fieldMap.at(fieldName).SetStoredValueRaw(field.m_StoredValueBuffer);
				}
			}
		}
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

	void ScriptEngine::OnImGuiRender()
	{
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
