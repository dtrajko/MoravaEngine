#pragma once

#include "ModelData.h"

#include <string>
#include <vector>


/**
 * A model file read through assimp, into plain data. The only place the model path meets assimp: the player of 10.3 reads
 * cooked models and doesn't link it.
 */
namespace ModelImport
{
	// False: the file couldn't be read (error says why). A file with only animations is fine: Meshes is empty then.
	// files: every file the import opened (an .obj's .mtl, a .gltf's .bin)
	bool Import(const std::string& path, ModelData& data, std::vector<std::string>& files, std::string& error);
}
