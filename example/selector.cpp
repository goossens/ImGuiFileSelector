//	FileSelector - A file select dialog for Dear ImGui.
//	Copyright (c) 2026 Johan A. Goossens. All rights reserved.
//
//	This work is licensed under the terms of the MIT license.
//	For a copy, see <https://opensource.org/licenses/MIT>.


//
//	Include files
//

#include <iomanip>
#include <random>
#include <sstream>

#include "selector.h"


//
//	Selector::render
//

void Selector::render() {
	// start window
	ImGuiWindowFlags windowFlags =
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoBringToFrontOnFocus;

	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
	ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);
	ImGui::Begin("Main Window", nullptr, windowFlags);
	// ImGui::PushFont(nullptr, 16.0f);

	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiKey_I, ImGuiInputFlags_RouteAlways)) {
		showDebugWindow = !showDebugWindow;
	}

	if (ImGui::Button("Open File")) {
		selector.OpenFile("Select File to Open...", "");
	}

	selector.Render();

	// ImGui::PopFont();
	ImGui::End();

	// show Dear ImGui metrics (if required)
	if (showDebugWindow) {
		ImGui::ShowMetricsWindow();
	}
}
