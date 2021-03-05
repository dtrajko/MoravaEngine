#pragma once

#include "../Scene/Entity.h"
#include "../Scene/Components.h"
#include "../Core/Timestep.h"

#include <string>
#include <unordered_map>


extern "C" {
	typedef struct _MonoObject MonoObject;
	typedef struct _MonoClassField MonoClassField;
}

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

		MonoObject* GetInstance();
	};

	struct PublicField
	{
		std::string Name;
		FieldType Type;

		PublicField(const std::string& name, FieldType type);
		PublicField(const PublicField&) = delete;
		PublicField(PublicField&& other);
		~PublicField();

		void CopyStoredValueToRuntime();
		bool IsRuntimeAvailable() const;

		template<typename T>
		T GetStoredValue() const
		{
			T value;
			GetStoredValue_Internal(&value);
			return value;
		}

		template<typename T>
		void SetStoredValue(T value) const
		{
			SetStoredValue_Internal(&value);
		}

		template<typename T>
		T GetRuntimeValue() const
		{
			T value;
			GetRuntimeValue_Internal(&value);
			return value;
		}

		template<typename T>
		void SetRuntimeValue(T value) const
		{
			SetRuntimeValue_Internal(&value);
		}

		void SetStoredValueRaw(void* src);
	private:
		EntityInstance* m_EntityInstance;
		MonoClassField* m_MonoClassField;
		uint8_t* m_StoredValueBuffer = nullptr;

		uint8_t* AllocateBuffer(FieldType type);
		void SetStoredValue_Internal(void* value) const;
		void GetStoredValue_Internal(void* outValue) const;
		void SetRuntimeValue_Internal(void* value) const;
		void GetRuntimeValue_Internal(void* outValue) const;

		friend class ScriptEngine;
	};

	using ScriptModuleFieldMap = std::unordered_map<std::string, std::unordered_map<std::string, PublicField>>;

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

		static void OnSceneDestruct(UUID sceneID);

		static void LoadHazelRuntimeAssembly(const std::string& path);
		static void ReloadAssembly(const std::string& path);

		static void SetSceneContext(const Ref<HazelScene>& scene);
		static const Ref<HazelScene>& GetCurrentSceneContext();

		static void CopyEntityScriptData(UUID dst, UUID src);

		static void OnCreateEntity(Entity entity);
		static void OnCreateEntity(UUID sceneID, UUID entityID);
		static void OnUpdateEntity(UUID sceneID, UUID entityID, Timestep ts);

		static void OnCollision2DBegin(Entity entity);
		static void OnCollision2DBegin(UUID sceneID, UUID entityID);
		static void OnCollision2DEnd(Entity entity);
		static void OnCollision2DEnd(UUID sceneID, UUID entityID);

		static void OnScriptComponentDestroyed(UUID sceneID, UUID entityID);

		static bool ModuleExists(const std::string& moduleName);
		static void InitScriptEntity(Entity entity);
		static void ShutdownScriptEntity(Entity entity, const std::string& moduleName);
		static void InstantiateEntityClass(Entity entity);

		static EntityInstanceMap& GetEntityInstanceMap();
		static EntityInstanceData& GetEntityInstanceData(UUID sceneID, UUID entityID);

		// Debug
		static void OnImGuiRender();
	};
}
