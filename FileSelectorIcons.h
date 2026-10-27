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
	static void RenderFile(ImDrawList* drawList, ImVec2 pos, float scale, const std::string& extension);
	static void RenderFolder(ImDrawList* drawList, ImVec2 pos, float scale);

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

	static constexpr Codepoint folder = u8"\uf101";
	static constexpr Codepoint file = u8"\uf102";
	static constexpr Codepoint fileWord = u8"\uf103";
	static constexpr Codepoint fileVideo = u8"\uf104";
	static constexpr Codepoint fileTable = u8"\uf105";
	static constexpr Codepoint filePowerpoint = u8"\uf106";
	static constexpr Codepoint fileMusic = u8"\uf107";
	static constexpr Codepoint fileImage = u8"\uf108";
	static constexpr Codepoint fileExcel = u8"\uf109";
	static constexpr Codepoint fileDocument = u8"\uf10a";
	static constexpr Codepoint fileDelimited = u8"\uf10b";
	static constexpr Codepoint fileCog = u8"\uf10c";
	static constexpr Codepoint fileCode = u8"\uf10d";
	static constexpr Codepoint fileChart = u8"\uf10e";
	static constexpr Codepoint fileCad = u8"\uf10f";

private:
	// font data
	static uint8_t iconData[3328];
	static constexpr size_t iconSize = 3328;

	// support functions
	static void render(ImDrawList* drawList, ImVec2 pos, float scale, Codepoint codepoint);
};
