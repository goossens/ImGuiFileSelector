//	FileSelector - A file select dialog for Dear ImGui.
//	Copyright (c) 2026 Johan A. Goossens. All rights reserved.
//
//	This work is licensed under the terms of the MIT license.
//	For a copy, see <https://opensource.org/licenses/MIT>.


#pragma once


//
//	Include files
//

#include <filesystem>
#include <string>

enum class KnownDirectory {
	desktop,
	documents,
	downloads,

	movies,
	music,
	pictures
};


//
//	Directory types
//



//
//	Functions
//

// move specified path to system-specific trash can
void getKnownDirectoryInfo(KnownDirectory type, std::string& label, std::filesystem::path& path);
