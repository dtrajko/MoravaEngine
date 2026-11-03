#include "ModelImport.h"

#include "Core/Log.h"
#include "Core/Math.h"

#include <assimp/DefaultIOSystem.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/Importer.hpp>
#include <assimp/LogStream.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <algorithm>
#include <cfloat>
#include <functional>
#include <unordered_map>


namespace ModelImport
{
	static const uint32_t s_ImportFlags =
		aiProcess_CalcTangentSpace |        // Create binormals/tangents just in case
		aiProcess_Triangulate |             // Make sure we're triangles
		aiProcess_SortByPType |             // Split meshes by primitive type
		aiProcess_GenNormals |              // Make sure we have legit normals
		aiProcess_GenUVCoords |             // Convert UVs if required
		aiProcess_OptimizeMeshes |          // Batch draws where possible
		aiProcess_ValidateDataStructure;    // Validation

	struct LogStream : public Assimp::LogStream
	{
		static void Initialize()
		{
			if (Assimp::DefaultLogger::isNullLogger())
			{
				Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);
				Assimp::DefaultLogger::get()->attachStream(new LogStream, Assimp::Logger::Err | Assimp::Logger::Warn);
			}
		}

		virtual void write(const char* message) override
		{
			// Assimp prefixes messages with "Warn, " or "Error, " and ends them with a newline
			std::string text(message);
			while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) text.pop_back();
			if (text.rfind("Warn", 0) == 0)
				Log::GetLogger()->warn("Assimp: {0}", text);
			else
				Log::GetLogger()->error("Assimp: {0}", text);
		}
	};

	// Notes what the importer opens: a cooked model is out of date when any of them changes
	class RecordingIOSystem : public Assimp::IOSystem
	{
	public:
		explicit RecordingIOSystem(std::vector<std::string>& files) : m_Files(files) {}

		bool Exists(const char* file) const override { return m_System.Exists(file); }
		char getOsSeparator() const override { return m_System.getOsSeparator(); }
		void Close(Assimp::IOStream* stream) override { m_System.Close(stream); }

		Assimp::IOStream* Open(const char* file, const char* mode = "rb") override
		{
			Assimp::IOStream* stream = m_System.Open(file, mode);
			if (stream)
			{
				m_Files.push_back(file);
			}
			return stream;
		}

	private:
		Assimp::DefaultIOSystem m_System;
		std::vector<std::string>& m_Files;
	};

	static void ReadMesh(const aiMesh* source, ModelData& data, uint32_t& vertexCount, uint32_t& indexCount)
	{
		ModelMeshData mesh;
		mesh.Name = source->mName.C_Str();
		mesh.BaseVertex = vertexCount;
		mesh.BaseIndex = indexCount;
		mesh.MaterialIndex = source->mMaterialIndex;
		mesh.IndexCount = source->mNumFaces * 3;
		mesh.VertexCount = source->mNumVertices;
		vertexCount += source->mNumVertices;
		indexCount += mesh.IndexCount;

		mesh.BoundsMin = glm::vec3(FLT_MAX);
		mesh.BoundsMax = glm::vec3(-FLT_MAX);

		for (uint32_t i = 0; i < source->mNumVertices; i++)
		{
			H2M::VertexH2M vertex = {};
			vertex.Position = { source->mVertices[i].x, source->mVertices[i].y, source->mVertices[i].z };
			if (source->HasNormals())
			{
				vertex.Normal = { source->mNormals[i].x, source->mNormals[i].y, source->mNormals[i].z };
			}
			mesh.BoundsMin = glm::min(mesh.BoundsMin, vertex.Position);
			mesh.BoundsMax = glm::max(mesh.BoundsMax, vertex.Position);
			if (source->HasTangentsAndBitangents())
			{
				vertex.Tangent = { source->mTangents[i].x, source->mTangents[i].y, source->mTangents[i].z };
				vertex.Binormal = { source->mBitangents[i].x, source->mBitangents[i].y, source->mBitangents[i].z };
			}
			if (source->HasTextureCoords(0))
			{
				vertex.Texcoord = { source->mTextureCoords[0][i].x, source->mTextureCoords[0][i].y };
			}

			if (data.IsAnimated)
			{
				H2M::AnimatedVertex animated;
				animated.Position = vertex.Position;
				animated.Normal = vertex.Normal;
				animated.Tangent = vertex.Tangent;
				animated.Binormal = vertex.Binormal;
				animated.Texcoord = vertex.Texcoord;
				data.AnimatedVertices.push_back(animated);
			}
			else
			{
				data.StaticVertices.push_back(vertex);
			}
		}

		for (uint32_t i = 0; i < source->mNumFaces; i++)
		{
			const aiFace& face = source->mFaces[i];
			if (face.mNumIndices != 3)
			{
				Log::GetLogger()->error("ModelH2M: the face contains invalid number of indices (expected: 3, detected: {0})!", face.mNumIndices);
				continue;
			}
			data.Indices.push_back({ face.mIndices[0], face.mIndices[1], face.mIndices[2] });
			mesh.TriangleCount++;
		}
		data.Meshes.push_back(std::move(mesh));
	}

	static void ReadBones(const aiScene* scene, const std::string& path, ModelData& data)
	{
		std::unordered_map<std::string, uint32_t> boneIndices;
		std::vector<uint32_t> trimmedVertices; // with more than four weights: the largest four are kept
		for (uint32_t m = 0; m < scene->mNumMeshes; m++)
		{
			const aiMesh* source = scene->mMeshes[m];
			ModelMeshData& mesh = data.Meshes[m];
			mesh.IsRigged = source->mNumBones > 0;

			for (uint32_t i = 0; i < source->mNumBones; i++)
			{
				const aiBone* bone = source->mBones[i];
				const std::string boneName(bone->mName.data);
				auto found = boneIndices.find(boneName);
				if (found == boneIndices.end())
				{
					found = boneIndices.emplace(boneName, (uint32_t)data.BoneNames.size()).first;
					data.BoneNames.push_back(boneName);
					data.BoneOffsets.push_back(Math::Mat4FromAssimpMat4(bone->mOffsetMatrix));
				}
				for (uint32_t j = 0; j < bone->mNumWeights; j++)
				{
					const size_t vertex = (size_t)mesh.BaseVertex + bone->mWeights[j].mVertexId;
					if (vertex < data.AnimatedVertices.size() && !data.AnimatedVertices[vertex].AddBoneData(found->second, bone->mWeights[j].mWeight))
					{
						trimmedVertices.push_back((uint32_t)vertex);
					}
				}
			}
		}
		if (!trimmedVertices.empty())
		{
			std::sort(trimmedVertices.begin(), trimmedVertices.end());
			trimmedVertices.erase(std::unique(trimmedVertices.begin(), trimmedVertices.end()), trimmedVertices.end());
			for (uint32_t vertex : trimmedVertices)
			{
				data.AnimatedVertices[vertex].NormalizeWeights();
			}
			Log::GetLogger()->info("ModelH2M: '{0}': {1} vertices have more than four bone weights: the largest four are kept", path, trimmedVertices.size());
		}
	}

	static std::string GetMap(const aiMaterial* material, aiTextureType type)
	{
		aiString file;
		return material->GetTexture(type, 0, &file) == AI_SUCCESS ? std::string(file.C_Str()) : std::string();
	}

	static ModelMaterialData ReadMaterial(const aiMaterial* source)
	{
		ModelMaterialData material;
		material.Name = source->GetName().C_Str();

		aiColor3D color;
		if (source->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
		{
			material.HasDiffuseColor = true;
			material.DiffuseColor = { color.r, color.g, color.b };
		}
		aiColor3D emissive;
		if (source->Get(AI_MATKEY_COLOR_EMISSIVE, emissive) == AI_SUCCESS)
		{
			material.HasEmissiveColor = true;
			material.EmissiveColor = { emissive.r, emissive.g, emissive.b };
		}
		material.HasShininess = source->Get(AI_MATKEY_SHININESS, material.Shininess) == AI_SUCCESS;
		material.HasReflectivity = source->Get(AI_MATKEY_REFLECTIVITY, material.Reflectivity) == AI_SUCCESS;
		material.HasMetallicFactor = source->Get(AI_MATKEY_METALLIC_FACTOR, material.MetallicFactor) == AI_SUCCESS;
		material.HasRoughnessFactor = source->Get(AI_MATKEY_ROUGHNESS_FACTOR, material.RoughnessFactor) == AI_SUCCESS;

		material.AlbedoMap = GetMap(source, aiTextureType_DIFFUSE);
		material.NormalMap = GetMap(source, aiTextureType_NORMALS);
		material.ShininessMap = GetMap(source, aiTextureType_SHININESS);
		material.DiffuseRoughnessMap = GetMap(source, aiTextureType_DIFFUSE_ROUGHNESS);
		material.MetalnessMap = GetMap(source, aiTextureType_METALNESS);
		material.EmissiveMap = GetMap(source, aiTextureType_EMISSIVE);
		material.AmbientOcclusionMap = GetMap(source, aiTextureType_AMBIENT_OCCLUSION);
		if (material.AmbientOcclusionMap.empty())
		{
			material.AmbientOcclusionMap = GetMap(source, aiTextureType_LIGHTMAP);
		}
		for (uint32_t p = 0; p < source->mNumProperties; p++)
		{
			const aiMaterialProperty* property = source->mProperties[p];
			if (property->mType == aiPTI_String && std::string(property->mKey.data) == "$raw.ReflectionFactor|file")
			{
				const uint32_t length = *(const uint32_t*)property->mData;
				material.ReflectionFactorMap.assign(property->mData + 4, length);
				break;
			}
		}
		return material;
	}

	static AnimationClip ReadClip(const aiAnimation* animation, uint32_t index)
	{
		AnimationClip clip;
		const std::string name = animation->mName.C_Str();
		clip.SetName(name.empty() ? "Animation " + std::to_string(index) : name);
		// The file's ticks become seconds (a file without a tick rate: 25 per second, as Assimp's)
		const double ticksPerSecond = animation->mTicksPerSecond != 0.0 ? animation->mTicksPerSecond : 25.0;
		clip.SetTicksPerSecond((float)ticksPerSecond);
		clip.SetDuration((float)(animation->mDuration / ticksPerSecond));

		std::vector<AnimationClip::Channel>& channels = clip.GetChannels();
		channels.reserve(animation->mNumChannels);
		for (uint32_t c = 0; c < animation->mNumChannels; c++)
		{
			const aiNodeAnim* source = animation->mChannels[c];
			AnimationClip::Channel channel;
			channel.Node = source->mNodeName.C_Str();
			for (uint32_t k = 0; k < source->mNumPositionKeys; k++)
			{
				const aiVectorKey& key = source->mPositionKeys[k];
				channel.PositionTimes.push_back((float)(key.mTime / ticksPerSecond));
				channel.Positions.emplace_back(key.mValue.x, key.mValue.y, key.mValue.z);
			}
			for (uint32_t k = 0; k < source->mNumRotationKeys; k++)
			{
				const aiQuatKey& key = source->mRotationKeys[k];
				channel.RotationTimes.push_back((float)(key.mTime / ticksPerSecond));
				channel.Rotations.push_back(glm::normalize(glm::quat(key.mValue.w, key.mValue.x, key.mValue.y, key.mValue.z)));
			}
			for (uint32_t k = 0; k < source->mNumScalingKeys; k++)
			{
				const aiVectorKey& key = source->mScalingKeys[k];
				channel.ScaleTimes.push_back((float)(key.mTime / ticksPerSecond));
				channel.Scales.emplace_back(key.mValue.x, key.mValue.y, key.mValue.z);
			}
			channels.push_back(std::move(channel));
		}
		return clip;
	}

	bool Import(const std::string& path, ModelData& data, std::vector<std::string>& files, std::string& error)
	{
		LogStream::Initialize();

		files.clear();
		Assimp::Importer importer;
		importer.SetIOHandler(new RecordingIOSystem(files)); // the importer owns it
		const aiScene* scene = importer.ReadFile(path, s_ImportFlags);
		if (!scene)
		{
			error = importer.GetErrorString();
			return false;
		}

		data = ModelData();
		// Animated models get the skinned vertex layout (bone IDs and weights)
		data.IsAnimated = scene->mAnimations != nullptr;

		uint32_t vertexCount = 0;
		uint32_t indexCount = 0;
		data.Meshes.reserve(scene->mNumMeshes);
		for (uint32_t m = 0; m < scene->mNumMeshes; m++)
		{
			ReadMesh(scene->mMeshes[m], data, vertexCount, indexCount);
		}

		std::function<void(const aiNode*, int)> addNode = [&](const aiNode* node, int parent)
		{
			const int index = (int)data.Nodes.size();
			data.Nodes.push_back({ node->mName.C_Str(), parent, Math::Mat4FromAssimpMat4(node->mTransformation) });
			data.NodeMeshes.emplace_back(node->mMeshes, node->mMeshes + node->mNumMeshes);
			for (uint32_t i = 0; i < node->mNumChildren; i++)
			{
				addNode(node->mChildren[i], index);
			}
		};
		if (scene->mRootNode)
		{
			addNode(scene->mRootNode, -1);
		}

		if (data.IsAnimated)
		{
			ReadBones(scene, path, data);
			for (uint32_t a = 0; a < scene->mNumAnimations; a++)
			{
				data.Clips.push_back(ReadClip(scene->mAnimations[a], a));
			}
		}

		data.Materials.reserve(scene->mNumMaterials);
		for (uint32_t i = 0; i < scene->mNumMaterials; i++)
		{
			data.Materials.push_back(ReadMaterial(scene->mMaterials[i]));
		}
		return true;
	}
}
