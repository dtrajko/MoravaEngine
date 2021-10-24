#pragma once

#include "HazelLegacy/Renderer/MeshHazelLegacy.h"


namespace Hazel
{

	struct MeshComponentHazelLegacy
	{
		Ref<MeshHazelLegacy> Mesh;

		bool CastShadows = true;    // MeshRenderer property in Unity
		bool ReceiveShadows = true; // MeshRenderer property in Unity

		MeshComponentHazelLegacy() = default;
		MeshComponentHazelLegacy(const MeshComponentHazelLegacy& other) = default;
		MeshComponentHazelLegacy(const Ref<MeshHazelLegacy>& mesh)
			: Mesh(mesh) {}

		operator Ref<MeshHazelLegacy>() { return Mesh; }
	};

}
