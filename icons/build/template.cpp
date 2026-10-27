//	FileSelector - A file select dialog for Dear ImGui.
//	Copyright (c) 2026 Johan A. Goossens. All rights reserved.
//
//	This work is licensed under the terms of the MIT license.
//	For a copy, see <https://opensource.org/licenses/MIT>.


//
//	Include files
//

#include <algorithm>
#include <unordered_map>

#include "FileSelectorIcons.h"


//
//	FileSelectorIcons::RenderFile
//

void FileSelectorIcons::RenderFile(ImDrawList* drawList, ImVec2 pos, float scale, const std::string& extension) {
	static std::unordered_map<std::string, Codepoint> icons = {
		{".c", fileCode},
		{".cpp", fileCode},
		{".h", fileCode},
		{".hpp", fileCode},
		{".lua", fileCode},
		{".py", fileCode},
		{".txt", fileDocument},
		{".md", fileDocument},
		{".json", fileCog},
		{".csv", fileDelimited},
		{".jpg", fileImage},
		{".jpeg", fileImage},
		{".png", fileImage},
		{".gif", fileImage},
		{".tiff", fileImage},
		{".svg", fileChart},
		{".mp4", fileVideo},
		{".mov", fileVideo},
		{".mp3", fileMusic},
		{".wav", fileMusic},
		{".aiff", fileMusic},
		{".doc", fileWord},
		{".docx", fileWord},
		{".xls", fileExcel},
		{".xlsx", fileExcel},
		{".ppt", filePowerpoint},
		{".pptx", filePowerpoint}
	};

	Codepoint codepoint = file;

	if (icons.find(extension) != icons.end()) {
		codepoint = icons[extension];
	}

	render(drawList, pos, scale, codepoint);
}


//
//	FileSelectorIcons::RenderFolder
//

void FileSelectorIcons::RenderFolder(ImDrawList* drawList, ImVec2 pos, float scale) {
	render(drawList, pos, scale, folder);
}


//
//	FileSelectorIcons::render
//

void FileSelectorIcons::render(ImDrawList* drawList, ImVec2 pos, float scale, Codepoint codepoint) {
	drawList->AddText(
		GetFont(),
		ImGui::GetStyle().FontSizeBase * scale,
		pos,
		ImGui::GetColorU32(ImGuiCol_TextDisabled),
		codepoint);
}


//
//	FileSelectorIcons::GetFont
//

ImFont* FileSelectorIcons::GetFont() {
	static ImFont* font = nullptr;

	if (!font) {
		ImFontConfig config;
		std::copy_n("Icons", 6, config.Name);
		config.FontDataOwnedByAtlas = false;

		font = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
			static_cast<void*>(GetData()),
			static_cast<int>(GetSize()),
			0.0f,
			&config);
	}

	return font;
}


//
//	Icons as embedded data
//

DATA
