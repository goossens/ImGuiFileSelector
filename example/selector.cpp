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
//	inputString
//

static bool inputString(const char* label, std::string* value) {
	// text input field with data from std::string
	ImGuiInputTextFlags flags =
		ImGuiInputTextFlags_NoUndoRedo |
		ImGuiInputTextFlags_CallbackResize;

	return ImGui::InputText(label, value->data(), value->capacity() + 1, flags, [](ImGuiInputTextCallbackData* data) {
		if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
			std::string* value = static_cast<std::string*>(data->UserData);
			value->resize(data->BufTextLen);
			data->Buf = (char*) value->c_str();
		}

		return 0;
	}, value);
}


//
//	Selector::render
//

void Selector::render() {
	// start window
	const ImGuiWindowFlags windowFlags =
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

	ImGui::SetNextItemWidth(ImGui::CalcTextSize("#").x * 30.0f);
	inputString("Extension filter", &extensionFilter);

	// trigger file selectors through a button or shortcut
	if (ImGui::Button("Open File") || ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O)) {
		selector.OpenFile(extensionFilter);
		logger.Add("Opened file selector");
	}

	ImGui::SameLine();

	if (ImGui::Button("Save As") || ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S)) {
		selector.SaveAs();
		logger.Add("Opened save as");
	}

	ImGui::SameLine();

	if (ImGui::Button("Select Files")) {
		selector.SelectFiles(extensionFilter);
		logger.Add("Opened select files");
	}

	ImGui::SameLine();

	if (ImGui::Button("Select Directory")) {
		selector.SelectDirectory();
		logger.Add("Opened select directory");
	}

	// render the selector each frame (if there is a need for it; handled internally)
	if (selector.Render()) {
		if (selector.SelectedOpenFile()) {
			logger.Add("File [" + selector.GetSelectedPath().string() + "] selected for open");

		} else if (selector.SelectedSaveAs()) {
			logger.Add("File [" + selector.GetSelectedPath().string() + "] selected for save as");

		} else if (selector.SelectedFiles()) {
			auto& paths = selector.GetSelectedPaths();
			logger.Add(paths.size() == 1 ? "File selected:" : "Files selected:");

			for (auto& path : paths) {
				logger.Add("[" + path.string() + "]");
			}

		} else if (selector.SelectedDirectory()) {
			logger.Add("Directory [" + selector.GetSelectedPath().string() + "] selected");

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
