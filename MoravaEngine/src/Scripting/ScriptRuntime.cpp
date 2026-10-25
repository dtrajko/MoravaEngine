#include "ScriptRuntime.h"

#include "ScriptHost.h"

#include "EnvMapVulkan/EnvMapVulkanScene.h"

#include "Core/Log.h"

#include <set>


// The entities with a script, in the order the Scene Hierarchy shows them: the order their scripts are made and called in
static void CollectScripted(const EnvMapVulkanScene& scene, const std::vector<EnvMapVulkanEntityID>& entities, std::vector<EnvMapVulkanEntityID>& out)
{
	for (EnvMapVulkanEntityID entity : entities)
	{
		if (scene.Has<ScriptComponent>(entity))
		{
			out.push_back(entity);
		}
		CollectScripted(scene, scene.GetChildren(entity), out);
	}
}

static std::string EntityName(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity)
{
	const NameComponent* name = scene.TryGet<NameComponent>(entity);
	return name ? name->Name : std::to_string(entity);
}

void ScriptRuntime::Start(EnvMapVulkanScene& scene)
{
	Stop();
	m_ExceptionCount = 0;
	m_LastException.clear();
	m_Scene = &scene; // also without scripts now: a prefab placed later may bring some

	std::vector<EnvMapVulkanEntityID> scripted;
	CollectScripted(scene, scene.GetRoots(), scripted);
	CreateInstances(scripted, true);
}

void ScriptRuntime::AddEntities(const std::vector<EnvMapVulkanEntityID>& entities)
{
	if (!m_Scene)
	{
		return;
	}
	std::vector<EnvMapVulkanEntityID> scripted;
	for (EnvMapVulkanEntityID entity : entities)
	{
		if (m_Scene->Has<ScriptComponent>(entity))
		{
			scripted.push_back(entity);
		}
	}
	CreateInstances(scripted, false);
}

void ScriptRuntime::CreateInstances(const std::vector<EnvMapVulkanEntityID>& scripted, bool atStart)
{
	if (scripted.empty())
	{
		return;
	}
	EnvMapVulkanScene& scene = *m_Scene;
	if (!ScriptHost::IsGameAssemblyLoaded())
	{
		Log::GetLogger()->error("Scripts: {0} entities have a script, but no game assembly is loaded ({1}): they don't run", scripted.size(),
			ScriptHost::IsAvailable() ? "Project Settings: Script Assembly" : "C# scripting isn't available");
		return;
	}

	// First every instance, then their fields (a field can refer to another entity's script), then each OnCreate
	const size_t first = m_Instances.size();
	std::set<std::string> classes;
	for (EnvMapVulkanEntityID entity : scripted)
	{
		const ScriptComponent& script = scene.Get<ScriptComponent>(entity);
		if (script.ClassName.empty())
		{
			Log::GetLogger()->warn("Scripts: '{0}' has a Script without a class: skipped", EntityName(scene, entity));
			continue;
		}
		if (!ScriptHost::FindClass(script.ClassName))
		{
			Log::GetLogger()->error("Scripts: '{0}' has the script '{1}', which isn't in the game's assembly (renamed, or not built yet)",
				EntityName(scene, entity), script.ClassName);
			continue;
		}
		void* handle = ScriptHost::CreateInstance(script.ClassName, entity);
		if (!handle)
		{
			continue; // C# logged why (its constructor threw)
		}
		m_ByEntity[entity] = m_Instances.size();
		Instance instance;
		instance.Entity = entity;
		instance.Handle = handle;
		instance.ClassName = script.ClassName;
		m_Instances.push_back(std::move(instance));
		classes.insert(script.ClassName);
	}
	const size_t end = m_Instances.size();
	for (size_t i = first; i < end; i++)
	{
		ApplyFields(m_Instances[i], scene.Get<ScriptComponent>(m_Instances[i].Entity));
	}
	if (atStart)
	{
		Log::GetLogger()->info("Scripts: {0} instance(s) of {1} class(es)", m_Instances.size(), classes.size());
	}
	// An OnCreate that places a prefab makes its instances (and calls their OnCreate) before it returns: not again here
	for (size_t i = first; i < end; i++)
	{
		if (!ScriptHost::InvokeCreate(m_Instances[i].Handle))
		{
			Threw(m_Instances[i], "OnCreate");
		}
	}
}

void ScriptRuntime::Stop()
{
	for (const Instance& instance : m_Instances)
	{
		ScriptHost::DestroyInstance(instance.Handle);
	}
	m_Instances.clear();
	m_ByEntity.clear();
	m_Scene = nullptr;
}

void ScriptRuntime::ApplyFields(const Instance& instance, const ScriptComponent& script)
{
	const ScriptClass* scriptClass = ScriptHost::FindClass(instance.ClassName);
	if (!scriptClass)
	{
		return;
	}
	for (const ScriptComponent::Field& stored : script.Fields)
	{
		const ScriptFieldInfo* field = scriptClass->FindField(stored.Name);
		if (!field)
		{
			Log::GetLogger()->warn("Scripts: '{0}' sets {1}.{2}, which the class doesn't have (any more): not used", EntityName(*m_Scene, instance.Entity),
				scriptClass->GetShortName(), stored.Name);
			continue;
		}
		ScriptFieldValue value;
		if (!stored.Value.ConvertTo(field->Type, value))
		{
			Log::GetLogger()->warn("Scripts: '{0}' sets {1}.{2} to a {3} value, but the field's type is {4} now: not used", EntityName(*m_Scene, instance.Entity),
				scriptClass->GetShortName(), stored.Name, ScriptFieldValue::GetTypeName(stored.Value.Type), field->TypeName);
			continue;
		}
		if (value.Type == ScriptFieldType::Entity && value.Entity != NoEntity && !m_Scene->Exists(value.Entity))
		{
			Log::GetLogger()->warn("Scripts: '{0}': {1}.{2} refers to an entity that isn't in the scene (removed): it stays null",
				EntityName(*m_Scene, instance.Entity), scriptClass->GetShortName(), stored.Name);
			value.Entity = NoEntity;
		}
		ScriptHost::SetField(instance.Handle, *field, value);
	}
}

// The update loops call the instances there were when they began: one a script places in them starts in the next update
// (its OnCreate has run). They go by index, as an instance placed during a call may move the others in memory

void ScriptRuntime::FixedUpdate(float fixedStep)
{
	const size_t count = m_Instances.size();
	for (size_t i = 0; i < count; i++)
	{
		if (!ScriptHost::InvokeFixedUpdate(m_Instances[i].Handle, fixedStep))
		{
			Threw(m_Instances[i], "OnFixedUpdate");
		}
	}
}

void ScriptRuntime::Update(float frameTime)
{
	const size_t count = m_Instances.size();
	for (size_t i = 0; i < count; i++)
	{
		if (!ScriptHost::InvokeUpdate(m_Instances[i].Handle, frameTime))
		{
			Threw(m_Instances[i], "OnUpdate");
		}
	}
}

void ScriptRuntime::DispatchEvents(const std::vector<ScenePhysics::Event>& events)
{
	if (m_Instances.empty())
	{
		return;
	}
	for (const ScenePhysics::Event& event : events)
	{
		ScriptCollisionKind kind = ScriptCollisionKind::CollisionEnter;
		const char* method = "OnCollisionEnter";
		switch (event.Type)
		{
		case ScenePhysics::EventType::CollisionEnter: kind = ScriptCollisionKind::CollisionEnter; method = "OnCollisionEnter"; break;
		case ScenePhysics::EventType::CollisionExit:  kind = ScriptCollisionKind::CollisionExit; method = "OnCollisionExit"; break;
		case ScenePhysics::EventType::TriggerEnter:   kind = ScriptCollisionKind::TriggerEnter; method = "OnTriggerEnter"; break;
		case ScenePhysics::EventType::TriggerExit:    kind = ScriptCollisionKind::TriggerExit; method = "OnTriggerExit"; break;
		}
		// Each side gets the other one; the normal points from the receiving entity towards the other. By index: a call
		// that places a prefab may move the instances in memory
		if (const Instance* instance = Find(event.Entity))
		{
			const size_t index = size_t(instance - m_Instances.data());
			if (!ScriptHost::InvokeCollision(instance->Handle, kind, event.Other, event.Point, event.Normal))
			{
				Threw(m_Instances[index], method);
			}
		}
		if (const Instance* instance = Find(event.Other))
		{
			const size_t index = size_t(instance - m_Instances.data());
			if (!ScriptHost::InvokeCollision(instance->Handle, kind, event.Entity, event.Point, -event.Normal))
			{
				Threw(m_Instances[index], method);
			}
		}
	}
}

void ScriptRuntime::RemoveDestroyed()
{
	if (!m_Scene)
	{
		return;
	}
	bool removed = false;
	for (size_t i = 0; i < m_Instances.size();)
	{
		if (m_Scene->Exists(m_Instances[i].Entity))
		{
			i++;
			continue;
		}
		ScriptHost::DestroyInstance(m_Instances[i].Handle);
		m_Instances.erase(m_Instances.begin() + i);
		removed = true;
	}
	if (removed)
	{
		m_ByEntity.clear();
		for (size_t i = 0; i < m_Instances.size(); i++)
		{
			m_ByEntity[m_Instances[i].Entity] = i;
		}
	}
}

void* ScriptRuntime::GetInstance(EnvMapVulkanEntityID entity) const
{
	const Instance* instance = Find(entity);
	return instance ? instance->Handle : nullptr;
}

const ScriptRuntime::Instance* ScriptRuntime::Find(EnvMapVulkanEntityID entity) const
{
	auto found = m_ByEntity.find(entity);
	return found != m_ByEntity.end() ? &m_Instances[found->second] : nullptr;
}

void ScriptRuntime::Threw(const Instance& instance, const char* method)
{
	m_ExceptionCount++;
	m_LastException = instance.ClassName + "." + method;
	instance.ExceptionCount++;
	instance.LastExceptionMethod = method;
	Log::GetLogger()->error("Script error: {0} on '{1}' threw an exception (above, with the file and line)", m_LastException,
		m_Scene ? EntityName(*m_Scene, instance.Entity) : std::to_string(instance.Entity));
}
