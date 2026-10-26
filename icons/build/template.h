//	FileSelector - A file select dialog for Dear ImGui.
//	Copyright (c) 2026 Johan A. Goossens. All rights reserved.
//
//	This work is licensed under the terms of the MIT license.
//	For a copy, see <https://opensource.org/licenses/MIT>.


#pragma once


//
//	Include files
//

#include <cstddef>
#include <cstdint>
#include <string>

#include "imgui.h"


//
//	FileSelectorIcons
//

class FileSelectorIcons {
public:
	// render icons (size is expressed in FontSizeBase units)
	// (e.g. size = 4.0f means four times as big as the current text)
	static void RenderFile(ImDrawList* drawList, ImVec2 pos, float size, const std::string& extension);
	static void RenderFolder(ImDrawList* drawList, ImVec2 pos, float size);

	// access icon data
	static ImFont* GetFont();
	inline static uint8_t* GetData() { return iconData; }
	inline static size_t GetSize() { return iconSize; }

	// icon codepoints
#if (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L) || (__cplusplus >= 202002L)
	using Codepoint = const char8_t*;
#else
	using Codepoint = const char*;
#endif

GLYPHS

private:
	// font data
	static uint8_t iconData[SIZE];
	static constexpr size_t iconSize = SIZE;

	// support functions
	static void render(ImDrawList* drawList, ImVec2 pos, float size, Codepoint codepoint);
};
