//	FileSelector - A file select dialog for Dear ImGui.
//	Copyright (c) 2026 Johan A. Goossens. All rights reserved.
//
//	This work is licensed under the terms of the MIT license.
//	For a copy, see <https://opensource.org/licenses/MIT>.


#pragma once


//
//	Include files
//

#include <string>
#include <vector>

#include "imgui.h"


//
//	Logger
//

class Logger {
public:
	// add a message to the log
	inline void Add(const std::string& message) { messages.emplace_back(message); }

	// clear message log
	inline void Clear() { messages.clear(); }

	// render message log
	inline void Render(const char* id, ImVec2 size=ImVec2()) {
		ImGui::BeginChild(id, size, ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);

		for (auto& message : messages) {
			ImGui::TextUnformatted(message.c_str());
		}

		if (scrollToBottom || (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())) {
			ImGui::SetScrollHereY(1.0f);
			scrollToBottom = false;
		}

		ImGui::EndChild();

		if (ImGui::BeginPopupContextItem("menu")) {
			if (ImGui::Checkbox("Auto scroll", &autoScroll)) {
				if (autoScroll) {
					scrollToBottom = true;
				}

				ImGui::CloseCurrentPopup();
			}

			if (ImGui::Button("Clear")) {
				messages.clear();
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::Button("Copy")) {
				std::string log;

				for (auto& message : messages) {
					log.append(message + "\n");
				}

				ImGui::SetClipboardText(log.c_str());
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

private:
	std::vector<std::string> messages;
	bool autoScroll = true;
	bool scrollToBottom = false;
};
