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

#include "knowndir.h"
#include "selector.h"
#include "trashcan.h"


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

	// shortcut to toggle Dear ImGui's debug window
	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiKey_I, ImGuiInputFlags_RouteAlways)) {
		showDebugWindow = !showDebugWindow;
	}

	// shortcut to toggle Dear ImGui's navigation mode
	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiKey_N, ImGuiInputFlags_RouteAlways)) {
		auto& io = ImGui::GetIO();

		if (io.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard) {
			io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
			io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;
			io.ConfigNavCursorVisibleAuto = true;
			io.ConfigNavCursorVisibleAlways = false;

		} else {
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
			io.ConfigNavCursorVisibleAuto = false;
			io.ConfigNavCursorVisibleAlways = true;
		}
	}

	// manage options
	if (ImGui::Checkbox("Show Sidebar", &showSideBar)) {
		selector.SetShowSideBar(showSideBar);
	}

	ImGui::SameLine();

	if (ImGui::Checkbox("Show Hidden", &showHidden)) {
		selector.SetShowHidden(showHidden);
	}

	// trigger file selectors through a button or shortcut
	if (ImGui::Button("Open File") || ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O, ImGuiInputFlags_RouteAlways)) {
		selector.OpenFile("Select File to Open...");
		logger.Add("Opened selector");
	}

	// render the selector each frame (if there is a need for it; handled internally)
	if (selector.Render()) {
		if (selector.SelectedOpenFile()) {
			logger.Add((std::string("File [") + selector.GetSelectedPath().string() + "] selected for open").c_str());

		} else if (selector.WasCancelled()) {
			logger.Add("Selector cancelled");
		}
	}

	logger.Render("Log");
	ImGui::End();

	// show Dear ImGui metrics (if required)
	if (showDebugWindow) {
		ImGui::ShowMetricsWindow();
	}
}
