#pragma once

#include <filesystem>
#include <string>


/**
 * A built game packed for downloading (docs/engine/Game_Engine_Runtime_Guide.html, 11.6): a Linux game is a .tar.gz of its
 * folder. Written by the engine itself, so a Windows editor makes one too: there, files have no executable bit, so the archive
 * gives it to every ELF file (the program, .NET's libraries and its createdump), whatever the disk says. Editor only.
 */
namespace GameArchive
{
	// The folder's files, under a top folder named topName, into a gzip-compressed tar file. False with the reason in error
	bool WriteTarGz(const std::filesystem::path& folder, const std::string& topName, const std::filesystem::path& archive, std::string& error);
}
