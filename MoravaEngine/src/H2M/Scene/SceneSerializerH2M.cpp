/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "SceneSerializerH2M.h"

#include "H2M/Core/AssertH2M.h"
#include "EntityH2M.h"
#include "ComponentsH2M.h"

#include <fstream>

#include <yaml-cpp/yaml.h>


namespace YAML {

	template<>
	struct convert<glm::vec2>
	{
		static Node encode(const glm::vec2& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec2& rhs)
		{
			if (!node.IsSequence() || node.size() != 2)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec3>
	{
		static Node encode(const glm::vec3& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec3& rhs)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec4>
	{
		static Node encode(const glm::vec4& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.push_back(rhs.w);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec4& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			rhs.w = node[3].as<float>();
			return true;
		}
	};

}
namespace H2M {

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
		return out;
	}

	static std::string RigidBody2DBodyTypeToString(Rigidbody2DComponentH2M::BodyType bodyType)
	{
		switch (bodyType)
		{
			case Rigidbody2DComponentH2M::BodyType::Static:    return "Static";
			case Rigidbody2DComponentH2M::BodyType::Dynamic:   return "Dynamic";
			case Rigidbody2DComponentH2M::BodyType::Kinematic: return "Kinematic";
		}

		H2M_CORE_ASSERT(false, "Unknown body type");
		return {};
	}

	static Rigidbody2DComponentH2M::BodyType RigidBody2DBodyTypeFromString(const std::string& bodyTypeString)
	{
		if (bodyTypeString == "Static")    return Rigidbody2DComponentH2M::BodyType::Static;
		if (bodyTypeString == "Dynamic")   return Rigidbody2DComponentH2M::BodyType::Dynamic;
		if (bodyTypeString == "Kinematic") return Rigidbody2DComponentH2M::BodyType::Kinematic;
	
		H2M_CORE_ASSERT(false, "Unknown body type");
		return Rigidbody2DComponentH2M::BodyType::Static;
	}

	SceneSerializerH2M::SceneSerializerH2M(const RefH2M<SceneH2M>& scene)
		: m_Scene(scene)
	{
	}

	static void SerializeEntity(YAML::Emitter& out, EntityH2M entity)
	{
		H2M_CORE_ASSERT(entity.HasComponent<IDComponentH2M>());

		out << YAML::BeginMap; // Entity
		out << YAML::Key << "Entity" << YAML::Value << entity.GetUUID();

		if (entity.HasComponent<TagComponentH2M>())
		{
			out << YAML::Key << "TagComponentH2M";
			out << YAML::BeginMap; // TagComponent

			auto& tag = entity.GetComponent<TagComponentH2M>().Tag;
			out << YAML::Key << "Tag" << YAML::Value << tag;

			out << YAML::EndMap; // TagComponent
		}

		if (entity.HasComponent<TransformComponentH2M>())
		{
			out << YAML::Key << "TransformComponentH2M";
			out << YAML::BeginMap; // TransformComponent

			auto& tc = entity.GetComponent<TransformComponentH2M>();
			out << YAML::Key << "Translation" << YAML::Value << tc.Translation;
			out << YAML::Key << "Rotation" << YAML::Value << tc.Rotation;
			out << YAML::Key << "Scale" << YAML::Value << tc.Scale;

			out << YAML::EndMap; // TransformComponent
		}

		if (entity.HasComponent<MeshComponentH2M>())
		{
			out << YAML::Key << "MeshComponentH2M";
			out << YAML::BeginMap; // MeshComponentH2M

			auto& mc = entity.GetComponent<MeshComponentH2M>();
			out << YAML::Key << "FilePath" << YAML::Value << mc.FilePath;
			out << YAML::Key << "CastShadows" << YAML::Value << mc.CastShadows;
			out << YAML::Key << "ReceiveShadows" << YAML::Value << mc.ReceiveShadows;

			out << YAML::EndMap; // MeshComponentH2M
		}

		if (entity.HasComponent<CameraComponentH2M>())
		{
			out << YAML::Key << "CameraComponentH2M";
			out << YAML::BeginMap; // CameraComponent

			auto& cameraComponent = entity.GetComponent<CameraComponentH2M>();
			auto& camera = cameraComponent.Camera;

			out << YAML::Key << "Camera" << YAML::Value;
			out << YAML::BeginMap; // Camera
			out << YAML::Key << "ProjectionType" << YAML::Value << (int)camera.GetProjectionType();
			out << YAML::Key << "PerspectiveFOV" << YAML::Value << camera.GetPerspectiveVerticalFOV();
			out << YAML::Key << "PerspectiveNear" << YAML::Value << camera.GetPerspectiveNearClip();
			out << YAML::Key << "PerspectiveFar" << YAML::Value << camera.GetPerspectiveFarClip();
			out << YAML::Key << "OrthographicSize" << YAML::Value << camera.GetOrthographicSize();
			out << YAML::Key << "OrthographicNear" << YAML::Value << camera.GetOrthographicNearClip();
			out << YAML::Key << "OrthographicFar" << YAML::Value << camera.GetOrthographicFarClip();
			out << YAML::EndMap; // Camera

			out << YAML::Key << "Primary" << YAML::Value << cameraComponent.Primary;
			out << YAML::Key << "FixedAspectRatio" << YAML::Value << cameraComponent.FixedAspectRatio;

			out << YAML::EndMap; // CameraComponent
		}

		if (entity.HasComponent<SpriteRendererComponentH2M>())
		{
			out << YAML::Key << "SpriteRendererComponentH2M";
			out << YAML::BeginMap; // SpriteRendererComponent

			auto& spriteRendererComponent = entity.GetComponent<SpriteRendererComponentH2M>();
			out << YAML::Key << "Color" << YAML::Value << spriteRendererComponent.Color;

			out << YAML::EndMap; // SpriteRendererComponent
		}

		if (entity.HasComponent<CircleRendererComponentH2M>())
		{
			out << YAML::Key << "CircleRendererComponentH2M";
			out << YAML::BeginMap; // CircleRendererComponent

			auto& circleRendererComponent = entity.GetComponent<CircleRendererComponentH2M>();
			out << YAML::Key << "Color" << YAML::Value << circleRendererComponent.Color;
			out << YAML::Key << "Thickness" << YAML::Value << circleRendererComponent.Thickness;
			out << YAML::Key << "Fade" << YAML::Value << circleRendererComponent.Fade;

			out << YAML::EndMap; // CircleRendererComponent
		}

		if (entity.HasComponent<Rigidbody2DComponentH2M>())
		{
			out << YAML::Key << "Rigidbody2DComponentH2M";
			out << YAML::BeginMap; // Rigidbody2DComponentH2M

			auto& rb2dComponent = entity.GetComponent<Rigidbody2DComponentH2M>();
			out << YAML::Key << "BodyType" << YAML::Value << RigidBody2DBodyTypeToString(rb2dComponent.Type);
			out << YAML::Key << "FixedRotation" << YAML::Value << rb2dComponent.FixedRotation;

			out << YAML::EndMap; // Rigidbody2DComponentH2M
		}

		if (entity.HasComponent<BoxCollider2DComponentH2M>())
		{
			out << YAML::Key << "BoxCollider2DComponentH2M";
			out << YAML::BeginMap; // BoxCollider2DComponent

			auto& bc2dComponent = entity.GetComponent<BoxCollider2DComponentH2M>();
			out << YAML::Key << "Offset"      << YAML::Value << bc2dComponent.Offset;
			out << YAML::Key << "Size"        << YAML::Value << bc2dComponent.Size;
			out << YAML::Key << "Density"     << YAML::Value << bc2dComponent.Density;
			out << YAML::Key << "Friction"    << YAML::Value << bc2dComponent.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << bc2dComponent.Restitution;
			out << YAML::Key << "RestitutionThreshold" << YAML::Value << bc2dComponent.RestitutionThreshold;

			out << YAML::EndMap; // BoxCollider2DComponent
		}

		if (entity.HasComponent<DirectionalLightComponentH2M>())
		{
			out << YAML::Key << "DirectionalLightComponentH2M";
			out << YAML::BeginMap; // DirectionalLightComponentH2M

			auto& directionalLightComponent = entity.GetComponent<DirectionalLightComponentH2M>();
			out << YAML::Key << "Radiance"    << YAML::Value << directionalLightComponent.Radiance;
			out << YAML::Key << "Intensity"   << YAML::Value << directionalLightComponent.Intensity;
			out << YAML::Key << "CastShadows" << YAML::Value << directionalLightComponent.CastShadows;
			out << YAML::Key << "SoftShadows" << YAML::Value << directionalLightComponent.SoftShadows;
			out << YAML::Key << "LightSize"   << YAML::Value << directionalLightComponent.LightSize;

			out << YAML::EndMap; // DirectionalLightComponentH2M
		}

		if (entity.HasComponent<PointLightComponentH2M>())
		{
			out << YAML::Key << "PointLightComponentH2M";
			out << YAML::BeginMap; // PointLightComponentH2M

			auto& pointLightComponent = entity.GetComponent<PointLightComponentH2M>();
			out << YAML::Key << "Enabled"          << YAML::Value << pointLightComponent.Enabled;
			out << YAML::Key << "Color"            << YAML::Value << pointLightComponent.Color;
			out << YAML::Key << "AmbientIntensity" << YAML::Value << pointLightComponent.AmbientIntensity;
			out << YAML::Key << "DiffuseIntensity" << YAML::Value << pointLightComponent.DiffuseIntensity;
			out << YAML::Key << "Constant"         << YAML::Value << pointLightComponent.Constant;
			out << YAML::Key << "Linear"           << YAML::Value << pointLightComponent.Linear;
			out << YAML::Key << "Exponent"         << YAML::Value << pointLightComponent.Exponent;
			out << YAML::Key << "FarPlane"         << YAML::Value << pointLightComponent.FarPlane;

			out << YAML::EndMap; // PointLightComponentH2M
		}

		if (entity.HasComponent<SpotLightComponentH2M>())
		{
			out << YAML::Key << "SpotLightComponentH2M";
			out << YAML::BeginMap; // SpotLightComponentH2M

			auto& spotLightComponent = entity.GetComponent<SpotLightComponentH2M>();
			out << YAML::Key << "Enabled"          << YAML::Value << spotLightComponent.Enabled;
			out << YAML::Key << "Color"            << YAML::Value << spotLightComponent.Color;
			out << YAML::Key << "AmbientIntensity" << YAML::Value << spotLightComponent.AmbientIntensity;
			out << YAML::Key << "DiffuseIntensity" << YAML::Value << spotLightComponent.DiffuseIntensity;
			out << YAML::Key << "Constant"         << YAML::Value << spotLightComponent.Constant;
			out << YAML::Key << "Linear"           << YAML::Value << spotLightComponent.Linear;
			out << YAML::Key << "Exponent"         << YAML::Value << spotLightComponent.Exponent;
			out << YAML::Key << "Edge"             << YAML::Value << spotLightComponent.Edge;
			out << YAML::Key << "EdgeProcessed"    << YAML::Value << spotLightComponent.EdgeProcessed;
			out << YAML::Key << "FarPlane"         << YAML::Value << spotLightComponent.FarPlane;

			out << YAML::EndMap; // SpotLightComponentH2M
		}

		out << YAML::EndMap; // Entity
	}

	void SceneSerializerH2M::Serialize(const std::string& filepath)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value << "Untitled";
		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
		m_Scene->m_Registry.each([&](auto entityID)
		{
			EntityH2M entity = { entityID, m_Scene.Raw() };
			if (!entity)
			{
				return;
			}

			SerializeEntity(out, entity);
		});
		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(filepath);
		fout << out.c_str();
	}

	void SceneSerializerH2M::SerializeRuntime(const std::string& filepath)
	{
		// Not implemented
		H2M_CORE_ASSERT(false);
	}

	bool SceneSerializerH2M::Deserialize(const std::string& filepath)
	{
		YAML::Node data;
		try
		{
			data = YAML::LoadFile(filepath);
		}
		catch (YAML::ParserException e)
		{
			return false;
		}

		if (!data["Scene"])
		{
			return false;
		}

		std::string sceneName = data["Scene"].as<std::string>();
		// H2M_CORE_TRACE("Deserializing scene '{0}'", sceneName);
		Log::GetLogger()->trace("Deserializing scene '{0}'", sceneName);

		auto entities = data["Entities"];
		if (entities)
		{
			for (auto entity : entities)
			{
				uint64_t uuid = entity["Entity"].as<uint64_t>();

				std::string name;
				auto tagComponent = entity["TagComponentH2M"];
				if (tagComponent)
				{
					name = tagComponent["Tag"].as<std::string>();
				}

				// H2M_CORE_TRACE("Deserialized entity with ID = {0}, name = {1}", uuid, name);
				Log::GetLogger()->trace("Deserialized entity with ID = {0}, name = {1}", uuid, name);

				EntityH2M deserializedEntity = m_Scene->CreateEntityWithUUID(uuid, name);

				auto transformComponent = entity["TransformComponentH2M"];
				if (transformComponent)
				{
					// Entities always have transforms
					auto& tc = deserializedEntity.GetComponent<TransformComponentH2M>();
					tc.Translation = transformComponent["Translation"].as<glm::vec3>();
					tc.Rotation    = transformComponent["Rotation"].as<glm::vec3>();
					tc.Scale       = transformComponent["Scale"].as<glm::vec3>();
				}

				auto meshComponent = entity["MeshComponentH2M"];
				if (meshComponent)
				{
					auto& mc = deserializedEntity.AddComponent<MeshComponentH2M>();
					mc.FilePath = meshComponent["FilePath"].as<std::string>();
					mc.CastShadows = meshComponent["CastShadows"].as<bool>();
					mc.ReceiveShadows = meshComponent["ReceiveShadows"].as<bool>();
				}

				auto cameraComponent = entity["CameraComponentH2M"];
				if (cameraComponent)
				{
					auto& cc = deserializedEntity.AddComponent<CameraComponentH2M>();

					auto& cameraProps = cameraComponent["Camera"];
					cc.Camera.SetProjectionType((SceneCameraH2M::ProjectionType)cameraProps["ProjectionType"].as<int>());

					cc.Camera.SetPerspectiveVerticalFOV(cameraProps["PerspectiveFOV"].as<float>());
					cc.Camera.SetPerspectiveNearClip(cameraProps["PerspectiveNear"].as<float>());
					cc.Camera.SetPerspectiveFarClip(cameraProps["PerspectiveFar"].as<float>());

					cc.Camera.SetOrthographicSize(cameraProps["OrthographicSize"].as<float>());
					cc.Camera.SetOrthographicNearClip(cameraProps["OrthographicNear"].as<float>());
					cc.Camera.SetOrthographicFarClip(cameraProps["OrthographicFar"].as<float>());

					cc.Primary = cameraComponent["Primary"].as<bool>();
					cc.FixedAspectRatio = cameraComponent["FixedAspectRatio"].as<bool>();
				}

				auto spriteRendererComponent = entity["SpriteRendererComponentH2M"];
				if (spriteRendererComponent)
				{
					auto& src = deserializedEntity.AddComponent<SpriteRendererComponentH2M>();
					src.Color = spriteRendererComponent["Color"].as<glm::vec4>();
				}

				auto circleRendererComponent = entity["CircleRendererComponentH2M"];
				if (circleRendererComponent)
				{
					auto& crc = deserializedEntity.AddComponent<CircleRendererComponentH2M>();
					crc.Color     = circleRendererComponent["Color"].as<glm::vec4>();
					crc.Thickness = circleRendererComponent["Thickness"].as<float>();
					crc.Fade      = circleRendererComponent["Fade"].as<float>();
				}

				auto rigidbody2DComponent = entity["Rigidbody2DComponentH2M"];
				if (rigidbody2DComponent)
				{
					auto& rb2d = deserializedEntity.AddComponent<Rigidbody2DComponentH2M>();
					rb2d.Type = RigidBody2DBodyTypeFromString(rigidbody2DComponent["BodyType"].as<std::string>());
					rb2d.FixedRotation = rigidbody2DComponent["FixedRotation"].as<bool>();
				}

				auto boxCollider2DComponent = entity["BoxCollider2DComponentH2M"];
				if (boxCollider2DComponent)
				{
					auto& bc2d = deserializedEntity.AddComponent<BoxCollider2DComponentH2M>();
					bc2d.Offset               = boxCollider2DComponent["Offset"].as<glm::vec2>();
					bc2d.Size                 = boxCollider2DComponent["Size"].as<glm::vec2>();
					bc2d.Density              = boxCollider2DComponent["Density"].as<float>();
					bc2d.Friction             = boxCollider2DComponent["Friction"].as<float>();
					bc2d.Restitution          = boxCollider2DComponent["Restitution"].as<float>();
					bc2d.RestitutionThreshold = boxCollider2DComponent["RestitutionThreshold"].as<float>();
				}

				auto directionalLightComponent = entity["DirectionalLightComponentH2M"];
				if (directionalLightComponent)
				{
					auto& dlc = deserializedEntity.AddComponent<DirectionalLightComponentH2M>();
					dlc.Radiance    = directionalLightComponent["Radiance"].as<glm::vec3>();
					dlc.Intensity   = directionalLightComponent["Intensity"].as<float>();
					dlc.CastShadows = directionalLightComponent["CastShadows"].as<bool>();
					dlc.SoftShadows = directionalLightComponent["SoftShadows"].as<bool>();
					dlc.LightSize   = directionalLightComponent["LightSize"].as<float>();
				}

				auto pointLightComponent = entity["PointLightComponentH2M"];
				if (pointLightComponent)
				{
					auto& plc = deserializedEntity.AddComponent<PointLightComponentH2M>();
					plc.Enabled          = pointLightComponent["Enabled"].as<bool>();
					plc.Color            = pointLightComponent["Color"].as<glm::vec3>();
					plc.AmbientIntensity = pointLightComponent["AmbientIntensity"].as<float>();
					plc.DiffuseIntensity = pointLightComponent["DiffuseIntensity"].as<float>();
					plc.Constant         = pointLightComponent["Constant"].as<float>();
					plc.Linear           = pointLightComponent["Linear"].as<float>();
					plc.Exponent         = pointLightComponent["Exponent"].as<float>();
					plc.FarPlane         = pointLightComponent["FarPlane"].as<float>();
				}

				auto spotLightComponent = entity["SpotLightComponentH2M"];
				if (spotLightComponent)
				{
					auto& slc = deserializedEntity.AddComponent<SpotLightComponentH2M>();
					slc.Enabled          = spotLightComponent["Enabled"].as<bool>();
					slc.Color            = spotLightComponent["Color"].as<glm::vec3>();
					slc.AmbientIntensity = spotLightComponent["AmbientIntensity"].as<float>();
					slc.DiffuseIntensity = spotLightComponent["DiffuseIntensity"].as<float>();
					slc.Constant         = spotLightComponent["Constant"].as<float>();
					slc.Linear           = spotLightComponent["Linear"].as<float>();
					slc.Exponent         = spotLightComponent["Exponent"].as<float>();
					slc.Edge             = spotLightComponent["Edge"].as<float>();
					slc.EdgeProcessed    = spotLightComponent["EdgeProcessed"].as<float>();
					slc.FarPlane         = spotLightComponent["FarPlane"].as<float>();
				}
			}
		}

		return true;
	}

	bool SceneSerializerH2M::DeserializeRuntime(const std::string& filepath)
	{
		// Not implemented
		H2M_CORE_ASSERT(false);
		return false;
	}

}
