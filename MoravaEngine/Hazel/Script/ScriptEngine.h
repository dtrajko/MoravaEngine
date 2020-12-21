#pragma once

#include "../Scene/Entity.h"
#include "../Scene/Components.h"
#include "../Core/Timestep.h"

#include <string>
#include <unordered_map>


//	extern "C" {
//		typedef struct _MonoObject MonoObject;
//		typedef struct _MonoClassField MonoClassField;
//	}

namespace Hazel
{

	enum class FieldType
	{
		None = 0, Float, Int, UnsignedInt, String, Vec2, Vec3, Vec4
	};

	const char* FieldTypeToString(FieldType type);

	struct EntityScriptClass;
	struct EntityInstance
	{
		EntityScriptClass* ScriptClass = nullptr;

		uint32_t Handle = 0;
		HazelScene* SceneInstance = nullptr;

		// MonoObject* GetInstance();
	};

	struct PublicField
	{
		std::string Name;
		FieldType Type;

		PublicField(const std::string& name, FieldType type);

		template<typename T>
		T GetValue() const
		{
			T value;
			GetValue_Internal(&value);
			return value;
		}

		template<typename T>
		void SetValue(T value) const
		{
			SetValue_Internal(&value);
		}

	private:
		EntityInstance* m_EntityInstance;
		// MonoClassField* m_MonoClassField;

		void SetValue_Internal(void* value) const;
		void GetValue_Internal(void* outValue) const;

		friend class ScriptEngine;
	};

	using ScriptModuleFieldMap = std::unordered_map<std::string, std::vector<PublicField>>;

	struct EntityInstanceData
	{
		EntityInstance Instance;
		ScriptModuleFieldMap ModuleFieldMap;
	};
	using EntityInstanceMap = std::unordered_map<UUID, std::unordered_map<UUID, EntityInstanceData>>;

	class ScriptEngine
	{
	public:
		static void Init(const std::string& assemblyPath);
		static void Shutdown();

		static void OnCreateEntity(Entity entity);
		static void OnUpdateEntity(UUID sceneID, uint32_t entityID, Timestep ts);

		static void InitScriptEntity(Entity entity);
		static void OnInitEntity(ScriptComponent& script, uint32_t entityID, uint32_t sceneID);

		static const ScriptModuleFieldMap& GetFieldMap();

		static void InstantiateEntityClass(Entity entity);

		static bool ModuleExists(std::string moduleName);

		static void CopyEntityScriptData(UUID dstSceneID, UUID srcSceneID);

		static EntityInstanceMap& GetEntityInstanceMap();
		static EntityInstanceData& GetEntityInstanceData(UUID sceneID, UUID entityID);

	};
}
