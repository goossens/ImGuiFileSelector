//	FileSelector - A file select dialog for Dear ImGui.
//	Copyright (c) 2026 Johan A. Goossens. All rights reserved.
//
//	This work is licensed under the terms of the MIT license.
//	For a copy, see <https://opensource.org/licenses/MIT>.


//
//	Include files
//

#include <algorithm>
#include <chrono>
#include <cmath>
#include <locale>
#include <sstream>

#include "FileSelector.h"


//
//	FileSelector::FileSelector
//

FileSelector::FileSelector() {
	setCurrentPath(std::filesystem::current_path(), false);
}


//
//	FileSelector::OpenFile
//

bool FileSelector::OpenFile(const char* label, const std::string&) {
	// don't do anything if a file selector is already open
	if (type != Type::idle) {
		return false;
	}

	// remember information and reset state
	type = Type::openFile;
	currentLabel = label;
	selectedPath.clear();
	isOpen = false;
	hasAction = false;
	clearSelections();

	pathHistory.clear();
	pathHistory.emplace_back(currentPath);
	historyIndex = 0;

	return true;
}


//
//	FileSelector::Render
//

bool FileSelector::Render() {
	// don't do anything if a file selector is not open yet
	if (type == Type::idle) {
		return false;
	}

	// ask popup to be opened (if required)
	if (!isOpen) {
		ImGui::OpenPopup(currentLabel.c_str());
		isOpen = true;
	}

	// render the selector popup
	auto viewPort = ImGui::GetMainViewport();
	ImVec2 center = viewPort->GetCenter();
	ImVec2 maxSize = viewPort->Size;
	ImVec2 minSize = maxSize * 0.6f;
	ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSizeConstraints(minSize, maxSize);

	ImGuiWindowFlags windowFlags =
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoScrollbar;

	if (ImGui::BeginPopupModal(currentLabel.c_str(), nullptr, windowFlags)) {
		renderFileDialog();

		// see if user performed action (selection or cancel)
		if (hasAction) {
			// handle open file scenario
			if (type == Type::openFile && !selectedPath.empty()) {
				// user selected a file
				// get directory of selected file
				auto directory = selectedPath.parent_path();

				// remove old recent places entry to avoid duplicates
				auto i = std::find(recentPlaces.begin(), recentPlaces.end(), directory);

				if (i != recentPlaces.end()) {
					recentPlaces.erase(i);
				}

				// limit list (if required) and add new entry
				if (recentPlaces.size() > 7) { recentPlaces.resize(7); }
				recentPlaces.emplace(recentPlaces.begin(), directory);
			}

			// close selector popup
			ImGui::CloseCurrentPopup();
			currentLabel.clear();
			type = Type::idle;
			isOpen = false;
		}

		// handle possible popups
		renderPopups();

		ImGui::EndPopup();
	}

	return hasAction;
}


//
//	FileSelector::setCurrentPath
//

bool FileSelector::setCurrentPath(const std::filesystem::path path, bool addHistory) {
	auto canonicalPath = std::filesystem::canonical(path);

	// sanity check
	if (!std::filesystem::exists(canonicalPath) || !std::filesystem::is_directory(canonicalPath)) {
		return false;
	}

	// try to refresh the path's node list and check for errors
	if (!refreshNodes(path)) {
		return false;
	}

	// sort directory entries
	sortNodes(sortColumn, sortDirection);

	// save new path
	currentPath = canonicalPath;

	// build a stack of path parts (in reverse order)
	pathStack.clear();
	std::filesystem::path partialPath;

	for (auto i = currentPath.begin(); i != currentPath.end(); i++) {
		partialPath /= *i;
		pathStack.emplace_back(i->u8string(), partialPath);
	}

	std::reverse(pathStack.begin(), pathStack.end());

	// add to history (if required)
	if (addHistory) {
		pathHistory.resize(historyIndex + 1);
		pathHistory.emplace_back(canonicalPath);
		historyIndex++;
	}

	return true;
}


//
//	FileSelector::refreshNodes
//

bool FileSelector::refreshNodes(const std::filesystem::path& path) {
	// we load to a temporary list first so we can detect errors
	std::vector<Node> tmpNodes;
	bool success = true;

	// get system locale
	std::locale locale("");

	// get the facets for wide characters (wstring)
	auto& ctypeFacet = std::use_facet<std::ctype<wchar_t>>(locale);
	auto& collateFacet = std::use_facet<std::collate<wchar_t>>(locale);

	try {
		for (const auto& entry : std::filesystem::directory_iterator(path)) {
			if (entry.is_regular_file() || entry.is_directory()) {
				if (showHiddenNodes || !isHidden(entry.path())) {
					// create a new node
					auto& node = tmpNodes.emplace_back();

					// get node metadata and set state
					node.path = entry.path();
					node.isDirectory = entry.is_directory();
					node.size = entry.is_regular_file() ? entry.file_size() : 0;
					node.lastUpdate = entry.last_write_time();
					node.isSelected = false;

					// precalculate strings for faster rendering
					node.pathString = node.path.filename().u8string();
					node.sizeString = node.isDirectory ? "    ---" : node.readableSize();
					node.updateString = node.readableDate(labels.timeFormat);

					// precalculate sort string
					auto sortString = node.path.filename().generic_wstring();
					ctypeFacet.tolower(sortString.data(), sortString.data() + sortString.size());
					node.sortString = collateFacet.transform(sortString.data(), sortString.data() + sortString.size());
				}
			}
		}

		nodes = tmpNodes;
		lastDirectoryWriteTime = std::filesystem::last_write_time(path);

	} catch (const std::filesystem::filesystem_error& e) {
		// create error message (handle path encoding)
		auto u8String = path.u8string();
		std::string utf8String(u8String.begin(), u8String.end());
		setErrorMessage(labels.cantAccess + " [" + utf8String + "]", e.what());
		success = false;
	}

	return success;
}


//
//	FileSelector::sortNodes
//

void FileSelector::sortNodes(ImS16 column, ImGuiSortDirection direction) {
	// remember settings
	sortColumn = column;
	sortDirection = direction;

	// sort current nodes
	std::sort(nodes.begin(), nodes.end(), [column, direction](const Node& left, const Node& right) {
		if (column == 0) {
			return (direction == ImGuiSortDirection_Ascending)
				? left.sortString < right.sortString
				: left.sortString > right.sortString;

		} else if (column == 1) {
			return (direction == ImGuiSortDirection_Ascending)
				? left.lastUpdate < right.lastUpdate
				: left.lastUpdate > right.lastUpdate;

		} else if (column == 2) {
			return (direction == ImGuiSortDirection_Ascending)
				? left.size < right.size
				: left.size > right.size;
		}

		return false;
	});
}


//
//	FileSelector::clearSelections
//

void FileSelector::clearSelections() {
	for (auto& node : nodes) {
		node.isSelected = false;
	}
}


//
//	FileSelector::renderFileDialog
//

void FileSelector::renderFileDialog() {
	// get reusable values
	frameHeight = ImGui::GetFrameHeight();
	glyphSize = ImGui::CalcTextSize("#");
	itemSpacing = ImGui::GetStyle().ItemSpacing;

	auto availableSpace = ImGui::GetContentRegionAvail();

	ImGuiChildFlags flags =
		ImGuiChildFlags_Borders |
		ImGuiChildFlags_ResizeX;

	if (ImGui::BeginChild("sideBar",ImVec2(glyphSize.x * 25.0f, availableSpace.y), flags)) {
		renderSideBar();
	}

	ImGui::EndChild();
	ImGui::SameLine();

	if (ImGui::BeginChild("mainArea", ImGui::GetContentRegionAvail())) {
		renderHeader();
		availableSpace = ImGui::GetContentRegionAvail();
		auto actionButtonHeight = frameHeight * 1.5f + itemSpacing.y * 2.0f;
		renderListView(ImVec2(availableSpace.x, availableSpace.y - actionButtonHeight));
		renderActionButtons();
	}

	ImGui::EndChild();
}


//
//	FileSelector::renderSideBar
//

void FileSelector::renderSideBar() {
	ImGui::TextDisabled("%s", labels.favorites.c_str());
	ImGui::Spacing();
	ImGui::TextDisabled("%s", labels.locations.c_str());
}


//
//	FileSelector::renderHeader
//

void FileSelector::renderHeader() {
	spacing();
	auto pos = ImGui::GetCursorScreenPos();
	auto availableSpace = ImGui::GetContentRegionAvail();

	auto disabled = historyIndex == 0;
	if (disabled) { ImGui::BeginDisabled(); }

	if (ImGui::ArrowButton("previous", ImGuiDir_Left)) {
		historyIndex--;
		setCurrentPath(pathHistory[historyIndex], false);
	}

	if (disabled) { ImGui::EndDisabled(); }

	ImGui::SameLine();
	disabled = historyIndex == pathHistory.size() - 1;
	if (disabled) { ImGui::BeginDisabled(); }

	if (ImGui::ArrowButton("next", ImGuiDir_Right)) {
		historyIndex++;
		setCurrentPath(pathHistory[historyIndex], false);
	}

	if (disabled) { ImGui::EndDisabled(); }

	ImGui::SetCursorScreenPos(ImVec2(pos.x + availableSpace.x * 0.25f, pos.y));
	float itemHeight = ImGui::GetTextLineHeightWithSpacing();
	float popupHeight = itemHeight * 12 + ImGui::GetStyle().FramePadding.y * 4.0f;
	ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, popupHeight));
	ImGui::SetNextItemWidth(availableSpace.x * 0.4f);

	if (ImGui::BeginCombo("###pathSelector", pathStack[0].name.c_str())) {
		for (auto i = pathStack.begin() + 1; i < pathStack.end(); i++) {
			ImGui::PushID(&(*i));

			if (ImGui::Selectable(reinterpret_cast<const char*>(i->name.c_str()))) {
				setCurrentPath(i->path);
			}

			ImGui::PopID();
		}

		ImGui::Separator();
		ImGui::TextDisabled("%s", labels.recentPlaces.c_str());

		for (auto i = recentPlaces.begin(); i < recentPlaces.end(); i++) {
			ImGui::PushID(&(*i));
			auto name = i->filename().u8string();

			if (ImGui::Selectable(reinterpret_cast<const char*>(name.c_str()))) {
				setCurrentPath(*i);
			}

			ImGui::PopID();
		}

		ImGui::EndCombo();
	}

	ImGui::SameLine();

	char buffer[256]={};
	ImGui::SetCursorScreenPos(ImVec2(pos.x + availableSpace.x * 0.75f, pos.y));
	ImGui::SetNextItemWidth(availableSpace.x * 0.25f);

	if (ImGui::InputTextWithHint("###search", labels.search.c_str(), buffer, sizeof(buffer))) {

	}

	spacing();
}


//
//	FileSelector::renderListView
//

void FileSelector::renderListView(ImVec2 size) {
	// build table of current nodes
	ImGuiTableFlags tableFlags =
		ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_RowBg |
		ImGuiTableFlags_BordersOuterH |
		ImGuiTableFlags_Sortable;

	if (ImGui::BeginTable("listView", 3, tableFlags, size)) {
		ImGui::TableSetupColumn(labels.nameColumn.c_str(), ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn(labels.dateColumn.c_str(), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize);
		ImGui::TableSetupColumn(labels.sizeColumn.c_str(), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize);
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableHeadersRow();

		// handle sort requests
		if (ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs()) {
			if (sortSpecs->SpecsDirty) {
				sortNodes(sortSpecs->Specs->ColumnIndex, sortSpecs->Specs->SortDirection);
				sortSpecs->SpecsDirty = false;
			}
		}

		// render each node
		for (auto& node : nodes) {
			ImGui::TableNextRow();

			// show filename
			ImGui::TableSetColumnIndex(0);

			ImGuiSelectableFlags selectableFlags =
				ImGuiSelectableFlags_SpanAllColumns |
				ImGuiSelectableFlags_AllowDoubleClick;

			if (ImGui::Selectable(reinterpret_cast<const char*>(node.pathString.c_str()), node.isSelected, selectableFlags)) {
				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
					if (node.isDirectory) {
						setCurrentPath(node.path);

					} else {
						selectedPath = node.path;
						hasAction = true;
					}

				} else {
					clearSelections();
					node.isSelected = true;
				}
			}

			if (ImGui::BeginPopupContextItem()) {
				if (ImGui::MenuItem(labels.rename.c_str()))   {}
				if (ImGui::MenuItem(labels.moveToTrash.c_str())) {}
				if (ImGui::MenuItem(labels.duplicate.c_str())) {}
				ImGui::EndPopup();
			}

			// show date
			ImGui::TableSetColumnIndex(1);
			ImGui::TextUnformatted(node.updateString.c_str());

			// show size
			ImGui::TableSetColumnIndex(2);
			ImGui::TextUnformatted(node.sizeString.c_str());
		}

		ImGui::EndTable();
	}
}


//
//	FileSelector::renderActionButtons
//

void FileSelector::renderActionButtons() {
	// a little vertical spacing
	spacing();

	// add ability to create a new folder
	if (ImGui::Button(labels.newFolder.c_str())) {

	}

	ImGui::SameLine();

	// right align buttons
	auto availableSpace = ImGui::GetContentRegionAvail();
	auto size = ImVec2((std::max(labels.ok.size(), labels.cancel.size()) + 2) * glyphSize.x, 0.0f);
	auto pos = ImGui::GetCursorScreenPos();

	ImGui::SetCursorScreenPos(ImVec2(pos.x + availableSpace.x - size.x * 2.0f - itemSpacing.x, pos.y));

	// handle cancel button and shortcut
	if (ImGui::Button(labels.cancel.c_str(), size)) {
		hasAction = true;
	}

	if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteAlways)) {
		hasAction = true;
	}

	// handle OK button
	ImGui::SameLine();

	if (selectedPath.empty()) {
		ImGui::BeginDisabled();
	}

	if (ImGui::Button(labels.ok.c_str(), size)) {
		hasAction = true;
	}

	if (selectedPath.empty()) {
		ImGui::EndDisabled();
	}
}


//
//	FileSelector::renderPopups
//

void FileSelector::renderPopups() {
	// handle error window
	if (openErrorMessage) {
		ImGui::OpenPopup(labels.errorWindow.c_str());
		openErrorMessage = false;
	}

	if (ImGui::BeginPopupModal(labels.errorWindow.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::TextUnformatted(errorMessage.c_str());

		if (errorDetails.size()) {
			ImGui::SetItemTooltip("%s", errorDetails.c_str());
		}

		if (ImGui::Button(labels.ok.c_str())) {
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}


//
//	FileSelector::spacing
//

void FileSelector::spacing() {
	auto pos = ImGui::GetCursorScreenPos();
	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + ImGui::GetFrameHeight() * 0.4f));
}


//
//	FileSelector::Node::readableSize
//

std::string FileSelector::Node::readableSize() {
	size_t i = 0;
	double mantissa = static_cast<double>(size);

	while (mantissa >= 1024.0) {
		mantissa /= 1024;
		i++;
	}

	mantissa = std::ceil(mantissa * 10.0) / 10.0;
	std::stringstream ss;
	ss << std::fixed << std::setw(5) << std::setprecision(1) << std::setfill(' ') << mantissa;
	ss << " KMGTPE"[i] << (i > 0 ? "B" : "");

	return ss.str();
}


//
//	FileSelector::Node::readableDate
//

std::string FileSelector::Node::readableDate(const std::string& format) {
	auto wallNow = std::chrono::system_clock::now();
	auto fileNow = std::filesystem::file_time_type::clock::now();

	auto duration = lastUpdate - fileNow;
	auto sysTime = wallNow + std::chrono::duration_cast<std::chrono::system_clock::duration>(duration);

	auto tt = std::chrono::system_clock::to_time_t(sysTime);
	std::tm localTime;

#ifdef _WIN32
	localtime_s(&localTime, &tt);

#else
	localtime_r(&tt, &localTime);
#endif

	std::stringstream ss;
	ss << std::put_time(&localTime, format.c_str());
	return ss.str();
}


//
//	FileSelector::isHidden
//

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifdef APIENTRY
#undef APIENTRY
#endif
#include <windows.h>
#endif

bool FileSelector::isHidden(const std::filesystem::path& path) {
	if (path.empty()) {
		return true;
	}

#ifdef _WIN32
	auto dwAttr = GetFileAttributesW(path.c_str());
	return (dwAttr == INVALID_FILE_ATTRIBUTES) ? false : ((dwAttr & FILE_ATTRIBUTE_HIDDEN) != 0);

#else
	PathString name = path.filename().u8string();
    return name[0] == '.' && name != "." && name != "..";
#endif
}
