//	FileSelector - A file select dialog for Dear ImGui.
//	Copyright (c) 2026 Johan A. Goossens. All rights reserved.
//
//	This work is licensed under the terms of the MIT license.
//	For a copy, see <https://opensource.org/licenses/MIT>.


#pragma once


//
//	Include files
//

#include "../FileSelector.h"

#include "logger.h"


//
//  Selector
//

class Selector {
public:
	// render a frame
	void render();

private:
	// properties
	FileSelector selector;
	Logger logger;
	bool showSideBar = true;
	bool showHidden = false;
	bool showDebugWindow = false;
	std::string extensionFilter;

	static constexpr const char* formats[] = { "PNG", "JPEG", "TIFF", "GIF" };
	static constexpr const char* formatExtensions[] = { ".png", ".jpeg", ".tiff", ".gif" };
	static constexpr size_t formatCount = sizeof(formats) / sizeof(formats[0]);
	size_t selectedFormat = 0;
};