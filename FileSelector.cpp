//	FileSelector - A file select dialog for Dear ImGui.
//	Copyright (c) 2026 Johan A. Goossens. All rights reserved.
//
//	This work is licensed under the terms of the MIT license.
//	For a copy, see <https://opensource.org/licenses/MIT>.


//
//	Include files
//

#if _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <locale>
#include <sstream>

#include "FileSelector.h"


//
//	FileSelector::FileSelector
//

FileSelector::FileSelector() {
	// set current path to current working directory
	setCurrentPath(std::filesystem::current_path());

	// add default sidebar links
	addDefaultFavorites();
	addDefaultClouds();
	addDefaultLocations();
	addDefaultMedia();
}


//
//	FileSelector::OpenFile
//

bool FileSelector::OpenFile(const std::string& filter) {
	return openDialog(Mode::openFile, filter);
}


//
//	FileSelector::SaveAs
//

bool FileSelector::SaveAs() {
	return openDialog(Mode::saveAs);
}


//
//	FileSelector::SelectFiles
//

bool FileSelector::SelectFiles(const std::string& filter) {
	return openDialog(Mode::selectFiles, filter);
}


//
//	FileSelector::SelectDirectory
//

bool FileSelector::SelectDirectory(const std::string& filter) {
	return openDialog(Mode::selectDirectory, filter);
}


//
//	FileSelector::openDialog
//

bool FileSelector::openDialog(Mode openMode, const std::string& filter) {
	if (mode == Mode::idle) {
		mode = openMode;
		saveAsString.clear();
		selectedPath.clear();
		selectedPaths.clear();
		listing.clearSelections();
		listing.setUserFilter("");
		listing.setExtensionFilter(filter);
		requestOpen = true;
		return true;

	} else {
		return false;
	}
}


//
//	FileSelector::Render
//

bool FileSelector::Render() {
	// don't do anything if a file selector isn't open yet
	if (mode == Mode::idle) {
		return false;
	}

	// ask popup to be opened (if required)
	if (requestOpen) {
		ImGui::OpenPopup("ImGuiFileSelector");
		requestOpen = false;
	}

	// render the selector popup
	auto viewPort = ImGui::GetMainViewport();
	ImVec2 center = viewPort->GetCenter();
	ImVec2 maxSize = viewPort->Size;
	ImVec2 minSize = maxSize * 0.6f;
	ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSizeConstraints(minSize, maxSize);

	ImGuiWindowFlags windowFlags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoScrollbar;

	if (ImGui::BeginPopupModal("###ImGuiFileSelector", nullptr, windowFlags)) {
		// handle dialog and possible popups
		action = Action::none;
		renderFileDialog();
		renderPopups();

		// see if user made selection
		if (action != Action::none && action != Action::cancelled) {
			// remove recent places entry to avoid duplication (if required)
			auto i = std::find(state.recentPlaces.begin(), state.recentPlaces.end(), state.currentPath);

			if (i != state.recentPlaces.end()) {
				state.recentPlaces.erase(i);
			}

			// limit list (if required)
			if (state.recentPlaces.size() > 7) {
				state.recentPlaces.resize(7);
			}

			// add current path to recent places
			state.recentPlaces.emplace(state.recentPlaces.begin(), state.currentPath);
		}

		if (action != Action::none) {
			// close selector popup
			ImGui::CloseCurrentPopup();
			mode = Mode::idle;
		}

		ImGui::EndPopup();
	}

	return action != Action::none;
}


//
//	FileSelector::setCurrentPath
//

bool FileSelector::setCurrentPath(const std::filesystem::path path, bool addHistory) {
	// convert path to an absolute, unique path with no relative elements (. or ..) or symbolic links
	std::error_code ec;
	auto canonicalPath = std::filesystem::canonical(path, ec);

	// sanity checks
	if (ec) {
		return false;
	}

	if (!isAccessible(canonicalPath)) {
		return false;
	}

	// try to refresh the directory listing and check for errors
	if (!listing.load(path, labels)) {
		return false;
	}

	// save new path
	state.currentPath = canonicalPath;

	// reset selections
	selectedPath.clear();
	selectedPaths.clear();

	// build a stack of path parts (in reverse order)
	pathStack.clear();
	std::filesystem::path partialPath;

	for (auto i = state.currentPath.begin(); i != state.currentPath.end(); i++) {
		partialPath /= *i;
		pathStack.emplace_back(pathToString(*i), partialPath);
	}

#ifdef _WIN32
	// handle root name/directory/path madness in Windows (thank you DOS :-)
	pathStack[1].name = pathToString(pathStack[1].path);
	pathStack.erase(pathStack.begin());
#endif

	std::reverse(pathStack.begin(), pathStack.end());

	// add to history (if required)
	if (addHistory) {
		pathHistory.resize(historyIndex);
		pathHistory.emplace_back(canonicalPath);
		historyIndex++;
	}

	return true;
}


//
//	FileSelector::renderFileDialog
//

void FileSelector::renderFileDialog() {
	// update path (if required)
	if (!nextPath.empty()) {
		setCurrentPath(nextPath);
		nextPath.clear();
	}

	// get reusable values
	frameHeight = ImGui::GetFrameHeight();
	glyphSize = ImGui::CalcTextSize("#");
	itemSpacing = ImGui::GetStyle().ItemSpacing;

	if (state.showSideBar) {
		auto availableSpace = ImGui::GetContentRegionAvail();

		ImGuiChildFlags flags =
			ImGuiChildFlags_Borders |
			ImGuiChildFlags_ResizeX;

		if (ImGui::BeginChild("sideBar",ImVec2(glyphSize.x * 25.0f, availableSpace.y), flags)) {
			renderSideBar();
		}

		ImGui::EndChild();
		ImGui::SameLine();
	}

	if (ImGui::BeginChild("mainArea", ImGui::GetContentRegionAvail())) {
		renderHeader();
		auto availableSpace = ImGui::GetContentRegionAvail();
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
	renderSideBarGroup(labels.favorites.c_str(), favorites);
	renderSideBarGroup(labels.clouds.c_str(), clouds);
	renderSideBarGroup(labels.locations.c_str(), locations);
	renderSideBarGroup(labels.media.c_str(), media);
}


//
//	FileSelector::renderSideBarGroup
//

void FileSelector::renderSideBarGroup(const std::string& label, SideBarGroup& group) {
	if (group.entries.size()) {
		grouping(label.c_str(), &(group.expanded));

		if (group.expanded) {
			ImGui::Indent();

			for (auto& entry : group.entries) {
				ImGui::PushID(&entry);

				if (ImGui::Selectable(reinterpret_cast<const char*>(entry.name.c_str()))) {
					nextPath = entry.path;
				}

				ImGui::PopID();
			}

			ImGui::Unindent();
		}

		ImGui::Spacing();
	}
}


//
//	FileSelector::renderHeader
//

void FileSelector::renderHeader() {
	auto width = ImGui::GetContentRegionAvail().x;
	spacing();

	if (mode == Mode::saveAs) {
		auto saveAsPos = ImGui::GetCursorScreenPos();
		auto saveAsWidth = glyphSize.x * 30.0f;
		auto totalWidth = ImGui::CalcTextSize(labels.saveAs.c_str()).x + itemSpacing.x + saveAsWidth;
		ImGui::SetCursorScreenPos(ImVec2(saveAsPos.x + (width - totalWidth) * 0.5f, saveAsPos.y));
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(labels.saveAs.c_str());
		ImGui::SameLine();
		ImGui::SetNextItemWidth(saveAsWidth);

		if (ImGui::IsWindowAppearing()) {
			ImGui::SetKeyboardFocusHere();
		}

		inputPath("###saveas", &saveAsString);
		spacing();
	}

	auto pos = ImGui::GetCursorScreenPos();
	auto disabled = historyIndex <= 1;
	if (disabled) { ImGui::BeginDisabled(); }

	if (ImGui::ArrowButton("previous", ImGuiDir_Left)) {
		historyIndex--;
		setCurrentPath(pathHistory[historyIndex - 1], false);
	}

	if (disabled) { ImGui::EndDisabled(); }

	ImGui::SameLine();
	disabled = historyIndex == pathHistory.size();
	if (disabled) { ImGui::BeginDisabled(); }

	if (ImGui::ArrowButton("next", ImGuiDir_Right)) {
		setCurrentPath(pathHistory[historyIndex++], false);
	}

	if (disabled) { ImGui::EndDisabled(); }

	ImGui::SetCursorScreenPos(ImVec2(pos.x + width * 0.25f, pos.y));
	float itemHeight = ImGui::GetTextLineHeightWithSpacing();
	float popupHeight = itemHeight * 12 + ImGui::GetStyle().FramePadding.y * 4.0f;
	ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, popupHeight));
	ImGui::SetNextItemWidth(width * 0.4f);

	if (ImGui::BeginCombo("###pathSelector", pathStack[0].name.c_str())) {
		for (auto i = pathStack.begin() + 1; i < pathStack.end(); i++) {
			ImGui::PushID(&(*i));

			if (ImGui::Selectable(reinterpret_cast<const char*>(i->name.c_str()))) {
				nextPath = i->path;
			}

			ImGui::PopID();
		}

		ImGui::Separator();
		ImGui::TextDisabled("%s", labels.recentPlaces.c_str());

		for (auto i = state.recentPlaces.begin(); i < state.recentPlaces.end(); i++) {
			ImGui::PushID(&(*i));
			auto name = pathToString(i->filename());

			if (ImGui::Selectable(reinterpret_cast<const char*>(name.c_str()))) {
				nextPath = *i;
			}

			ImGui::PopID();
		}

		ImGui::EndCombo();
	}

	ImGui::SameLine();
	ImGui::SetCursorScreenPos(ImVec2(pos.x + width * 0.75f, pos.y));
	ImGui::SetNextItemWidth(width * 0.25f);

	if (inputStringWithHint("###filter", labels.filter.c_str(), &filterString)) {
		listing.setUserFilter(filterString);
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
				sortSpecs->SpecsDirty = false;
				state.sortColumn = static_cast<SortColumn>(sortSpecs->Specs->ColumnIndex);
				state.sortOrder	= sortSpecs->Specs->SortDirection == ImGuiSortDirection_Ascending ? SortOrder::ascending : SortOrder::descending;
				listing.setSort(state.sortColumn, state.sortOrder);
			}
		}

		// render each entry
		listing.forEach([this](Entry& entry) {
			ImGui::TableNextRow();

			// show filename
			ImGui::TableSetColumnIndex(0);

			ImGuiSelectableFlags selectableFlags =
				ImGuiSelectableFlags_SpanAllColumns |
				ImGuiSelectableFlags_AllowDoubleClick;

			if (ImGui::Selectable(reinterpret_cast<const char*>(entry.nameString.c_str()), entry.isSelected, selectableFlags)) {
				handleEntrySelection(entry);
			}

			if (ImGui::BeginPopupContextItem()) {
				if (ImGui::MenuItem(labels.rename.c_str())) {}
				if (ImGui::MenuItem(labels.moveToTrash.c_str())) {}
				if (ImGui::MenuItem(labels.duplicate.c_str())) {}
				ImGui::EndPopup();
			}

			// show date
			ImGui::TableSetColumnIndex(1);
			ImGui::TextUnformatted(entry.updateString.c_str());

			// show size
			ImGui::TableSetColumnIndex(2);
			ImGui::TextUnformatted(entry.sizeString.c_str());
		});

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

	// select label of "OK" button
	std::string& okLabel = (mode == Mode::openFile) ? labels.open : (mode == Mode::saveAs) ? labels.save : labels.select;

	// right align buttons
	auto pos = ImGui::GetCursorScreenPos();
	auto availableSpace = ImGui::GetContentRegionAvail();

	auto size = ImVec2(
		std::max(
			ImGui::CalcTextSize(okLabel.c_str()).x,
			ImGui::CalcTextSize(labels.cancel.c_str()).x) + glyphSize.x * 2.0f,
		0.0f);

	ImGui::SetCursorScreenPos(ImVec2(pos.x + availableSpace.x - size.x * 2.0f - itemSpacing.x, pos.y));

	// handle cancel button and shortcut
	if (ImGui::Button(labels.cancel.c_str(), size) || ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteAlways)) {
		selectedPath.clear();
		selectedPaths.clear();
		action = Action::cancelled;
	}

	// handle OK button (disable when nothing is selected)
	ImGui::SameLine();
	auto okAvailable = isOkAvailable();

	if (!okAvailable) {
		ImGui::BeginDisabled();
	}

	if (ImGui::Button(okLabel.c_str(), size) || (okAvailable && ImGui::Shortcut(ImGuiKey_Enter, ImGuiInputFlags_RouteAlways))) {
		handleOk();
	}

	if (!okAvailable) {
		ImGui::EndDisabled();
	}
}


//
//	FileSelector::renderPopups
//

void FileSelector::renderPopups() {
	// handle overwrite window
		if (openOverWrite) {
		ImGui::OpenPopup(labels.confirmationWindow.c_str());
		openOverWrite = false;
	}

	if (ImGui::BeginPopupModal(labels.confirmationWindow.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::TextUnformatted(labels.fileExists.c_str());

		if (ImGui::Button(labels.cancel.c_str())) {
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button(labels.ok.c_str())) {
			action = Action::selectedSaveAs;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

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
//	FileSelector::addDefaultFavorites
//

void FileSelector::addDefaultFavorites() {
	auto home = getHome();

	if (!home.empty()) {
		// these only get added when they exist
		favorites.add("Home", home);
		favorites.add(KnownDirectory::desktop);
		favorites.add(KnownDirectory::documents);
		favorites.add(KnownDirectory::downloads);
	}
}


//
//	FileSelector::addDefaultClouds
//

void FileSelector::addDefaultClouds() {
	auto home = getHome();

	if (!home.empty()) {
		clouds.add("iCloud Drive", home / "Library" / "Mobile Documents" / "com~apple~CloudDocs");
		clouds.add("OneDrive", home / "OneDrive");
	}
}


//
//	FileSelector::addDefaultLocations
//

void FileSelector::addDefaultLocations() {
	forEachKnownLocation([this](const std::string &name, const std::filesystem::path &path) {
		 locations.add(name, path);
	});
}


//
//	FileSelector::addDefaultMedia
//

void FileSelector::addDefaultMedia() {
	auto home = getHome();

	if (!home.empty()) {
		media.add(KnownDirectory::movies);
		media.add(KnownDirectory::music);
		media.add(KnownDirectory::pictures);
	}
}


//
//	FileSelector::handleEntrySelection
//

void FileSelector::handleEntrySelection(Entry& entry) {
	if (entry.isDirectory) {
		if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
			nextPath = entry.path;

		} else if (mode == Mode::selectDirectory) {
			listing.clearSelections();
			selectedPath = entry.path;
			entry.isSelected = true;
		}

	} else {
		switch (mode) {
			case Mode::openFile: {
				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
					selectedPath = entry.path;
					action = Action::selectedOpenFile;

				} else if (entry.isSelected) {
					listing.clearSelections();
					selectedPath.clear();
					entry.isSelected = false;

				} else {
					listing.clearSelections();
					selectedPath = entry.path;
					entry.isSelected = true;
				}

				break;
			}

			case Mode::saveAs: {
				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
					selectedPath = entry.path;

					if (std::filesystem::exists(selectedPath)) {
						openOverWrite = true;

					} else {
						action = Action::selectedSaveAs;
					}

				} else {
					saveAsString = entry.nameString;
				}

				break;
			}

			case Mode::selectFiles: {
				if (entry.isSelected) {
					selectedPaths.erase(
						std::remove(
							selectedPaths.begin(),
							selectedPaths.end(),
							entry.path),
						selectedPaths.end());

					entry.isSelected = false;

				} else {
					selectedPaths.emplace_back(entry.path);
					entry.isSelected = true;
				}

				break;
			}

			default:
				break;
		}
	}
}


//
//	FileSelector::isOkAvailable
//

bool FileSelector::isOkAvailable() {
	switch (mode) {
		case Mode::openFile: return !selectedPath.empty(); break;
		case Mode::saveAs: return saveAsString.size(); break;
		case Mode::selectFiles: return selectedPaths.size(); break;
		case Mode::selectDirectory: return true; break;
		default: break;
	}

	return false;
}


//
//	FileSelector::handleOk
//

void FileSelector::handleOk() {
	switch (mode) {
		case Mode::openFile:
			action = Action::selectedOpenFile;
			break;

		case Mode::saveAs:
			selectedPath = state.currentPath / saveAsString;

			if (std::filesystem::exists(selectedPath)) {
				openOverWrite = true;

			} else {
				action = Action::selectedSaveAs;
			}

			break;

		case Mode::selectFiles:
			action = Action::selectedFiles;
			break;

		case Mode::selectDirectory:
			if (selectedPath.empty()) {
				selectedPath = state.currentPath;
			}

			action = Action::selectedDirectory;
			break;

		default:
			break;
	}
}


//
//	FileSelector::spacing
//

void FileSelector::spacing() {
	auto pos = ImGui::GetCursorScreenPos();
	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + frameHeight * 0.4f));
}


//
//	FileSelector::grouping
//

bool FileSelector::grouping(const char* label, bool* expanded) {
	// determine position and space
	auto pos = ImGui::GetCursorScreenPos();
	auto size = ImGui::GetContentRegionAvail();
	size.y = glyphSize.y;

	// run button action
	bool changed = ImGui::InvisibleButton(label, size);

	if (changed) {
		*expanded = !*expanded;
	}

	// render label and state
	auto drawList = ImGui::GetWindowDrawList();
	auto color = ImGui::GetColorU32(ImGuiCol_TextDisabled);
	drawList->AddText(pos, color, label);

	if (ImGui::IsItemHovered()) {
		auto right = pos + ImVec2(size.x - glyphSize.x, 0.0f);
		ImVec2 p1 = ImVec2(right + ImVec2(0.0f, glyphSize.y * 0.3f));
		ImVec2 p2 = right + (*expanded ? ImVec2(glyphSize.x * 0.5f, glyphSize.y * 0.7f) : ImVec2(glyphSize.x, glyphSize.y * 0.5f));
		ImVec2 p3 = right + (*expanded ? ImVec2(glyphSize.x, glyphSize.y * 0.3f) : ImVec2(0.0f, glyphSize.y * 0.7f));
		drawList->AddLine(p1, p2, color);
		drawList->AddLine(p2, p3, color);
	}

	// run result
	return changed;
}


//
//	FileSelector::inputString
//

bool FileSelector::inputString(const char* label, std::string* value) {
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
//	FileSelector::inputStringWithHint
//

bool FileSelector::inputStringWithHint(const char* label, const char* hint, std::string* value) {
	ImGuiInputTextFlags flags =
		ImGuiInputTextFlags_NoUndoRedo |
		ImGuiInputTextFlags_CallbackResize;

	return ImGui::InputTextWithHint(label, hint, value->data(), value->capacity() + 1, flags, [](ImGuiInputTextCallbackData* data) {
		if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
			std::string* value = static_cast<std::string*>(data->UserData);
			value->resize(data->BufTextLen);
			data->Buf = (char*) value->c_str();
		}

		return 0;
	}, value);
}


//
//	FileSelector::inputPath
//

bool FileSelector::inputPath(const char* label, std::string* value) {
	ImGuiInputTextFlags flags =
		ImGuiInputTextFlags_NoUndoRedo |
		ImGuiInputTextFlags_CallbackResize |
		ImGuiInputTextFlags_CallbackCharFilter;

	return ImGui::InputText(label, value->data(), value->capacity() + 1, flags, [](ImGuiInputTextCallbackData* data) {
		if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
			std::string* value = static_cast<std::string*>(data->UserData);
			value->resize(data->BufTextLen);
			data->Buf = (char*) value->c_str();

		} else if (data->EventFlag == ImGuiInputTextFlags_CallbackCharFilter) {
			// MacOS illegal: /:
			// Linux illegal: /
			// Windows illegal: <>:\"/\\|?*
			// web undesirable: %&#+={}
			static const std::string illegalChars = "<>:\"/\\|?*%&#+={}";

			if (illegalChars.find(static_cast<char>(data->EventChar)) != std::string::npos) {
				return 1;
			}
		}

		return 0;
	}, value);
}


//
//	FileSelector::Listing::load
//

bool FileSelector::Listing::load(const std::filesystem::path& path, const Labels& labels) {
	// we load to a temporary list first so we can detect errors
	std::vector<Entry> entries;
	bool success = true;

	// get system locale
	std::locale locale("");

	// get the facets for wide characters (wstring)
	auto& ctypeFacet = std::use_facet<std::ctype<wchar_t>>(locale);
	auto& collateFacet = std::use_facet<std::collate<wchar_t>>(locale);

	try {
		error = "";

		for (const auto& node : std::filesystem::directory_iterator(path)) {
			if (node.is_regular_file() || (node.is_directory() && isAccessible(node.path()))) {
				// create a new entry
				auto& entry = entries.emplace_back();

				// get entry metadata and set state
				entry.path = node.path();
				entry.isDirectory = node.is_directory();
				entry.size = node.is_regular_file() ? node.file_size() : 0;
				entry.extension = node.is_regular_file() ? pathToString(entry.path.extension()) : "";
				entry.lastUpdate = node.last_write_time();
				entry.isHidden = isHidden(entry.path);
				entry.isSelected = false;

				// precalculate strings for faster rendering
				entry.nameString = pathToString(entry.path.filename());
				entry.sizeString = entry.isDirectory ? "    ---" : entry.readableSize(labels);
				entry.updateString = entry.readableDate(labels);

				// precalculate sort string
				auto sortString = entry.path.filename().generic_wstring();
				ctypeFacet.tolower(sortString.data(), sortString.data() + sortString.size());
				entry.sortString = collateFacet.transform(sortString.data(), sortString.data() + sortString.size());
			}
		}

		clear();

		for (auto& entry : entries) {
			emplace_back(entry);
		}

		sort();
		currentPath = path;
		lastWriteTime = std::filesystem::last_write_time(path);

	} catch (const std::filesystem::filesystem_error& e) {
		error = e.what();
		success = false;
	}

	return success;
}


//
//	FileSelector::Listing::setSort
//

void FileSelector::Listing::setSort(SortColumn column, SortOrder order) {
	sortColumn = column;
	sortOrder = order;
	sort();
}


//
//	FileSelector::Listing::setExtensionFilter
//

void FileSelector::Listing::setExtensionFilter(const std::string& filter) {
	extensions.clear();
	std::stringstream ss(filter);
	std::string extension;

	while (std::getline(ss, extension, ',')) {
		extensions.emplace_back(extension);
	}
}


//
//	FileSelector::Listing::setUserFilter
//

void FileSelector::Listing::setUserFilter(const std::string& filter) {
	if (filter.size()) {
		try {
			filterRegex.assign(filter, std::regex_constants::icase);
			filterActive = true;
			filterValid = true;
			error.clear();

		} catch (const std::regex_error& e) {
			filterActive = false;
			filterValid = false;
			error = e.what();
		}

	} else {
		filterRegex = std::regex();
		filterActive = false;
		error.clear();
	}
}


//
//	FileSelector::Listing::forEach
//

void FileSelector::Listing::forEach(std::function<void(Entry&)> callback) {
	for (auto& entry : *this) {
		if (filter(entry)) {
			callback(entry);
		}
	}
}


//
//	FileSelector::Listing::clearSelections
//

void FileSelector::Listing::clearSelections() {
	for (auto& entry : *this) {
		entry.isSelected = false;
	}
}


//
//	FileSelector::Listing::sort
//

void FileSelector::Listing::sort() {
	// sort current nodes
	std::sort(begin(), end(), [this](const Entry& left, const Entry& right) {
		if (sortColumn == SortColumn::name) {
			return (sortOrder == SortOrder::ascending)
				? left.sortString < right.sortString
				: left.sortString > right.sortString;

		} else if (sortColumn == SortColumn::date) {
			return (sortOrder == SortOrder::ascending)
				? left.lastUpdate < right.lastUpdate
				: left.lastUpdate > right.lastUpdate;

		} else if (sortColumn == SortColumn::size) {
			return (sortOrder == SortOrder::ascending)
				? left.size < right.size
				: left.size > right.size;
		}

		return false;
	});
}


//
//	FileSelector::Listing::filter
//

bool FileSelector::Listing::filter(const Entry& entry) {
	// filter by visibility
	if (!showHidden && entry.isHidden) {
		return false;
	}

	// filter by extension
	if (extensions.size()) {
		if (std::find(extensions.begin(), extensions.end(), entry.extension) != extensions.end()) {
			return false;
		}
	}

	// filter by user request
	if (filterActive && !std::regex_search(entry.nameString, filterRegex)) {
		return false;
	}

	return true;
}


//
//	FileSelector::Entry::readableSize
//

std::string FileSelector::Entry::readableSize(const Labels& labels) {
	size_t i = 0;
	double mantissa = static_cast<double>(size);

	while (mantissa >= 1024.0) {
		mantissa /= 1024;
		i++;
	}

	mantissa = std::ceil(mantissa * 10.0) / 10.0;
	std::stringstream ss;
	ss << std::fixed << std::setw(5) << std::setprecision(1) << std::setfill(' ') << mantissa;

	switch (i) {
		case 0: ss << labels.bytes; break;
		case 1: ss << labels.kiloBytes; break;
		case 2: ss << labels.megaBytes; break;
		case 3: ss << labels.gigaBytes; break;
		case 4: ss << labels.terraBytes; break;
		case 5: ss << labels.petaBytes; break;
		case 6: ss << labels.exoBytes; break;
		default: ss << "??"; break;
	}

	return ss.str();
}


//
//	FileSelector::Entry::readableDate
//

std::string FileSelector::Entry::readableDate(const Labels& labels) {
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
	ss << std::put_time(&localTime, labels.timeFormat.c_str());
	return ss.str();
}


//
//	Operating System specific functions
//

#ifdef _WIN32
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifdef APIENTRY
#undef APIENTRY
#endif

#include <cstdlib>

#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

#pragma comment(lib, "Shell32.lib")

#else
#include <cstdlib>
#include <pwd.h>
#include <unistd.h>

#ifdef __APPLE__
#include <sys/stat.h>

#include <CoreFoundation/CoreFoundation.h>

#include <objc/runtime.h>
#include <objc/message.h>

extern "C" {
	void *objc_autoreleasePoolPush(void);
	void objc_autoreleasePoolPop(void* pool);
}

#else
#include <fstream>
#include <mntent.h>
#endif

#endif


//
//	FileSelector::getHome
//

std::filesystem::path FileSelector::getHome() {
#ifdef _WIN32
	PWSTR wcharPath = nullptr;

	// preferred Windows API over getenv("USERPROFILE")
	if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Profile, KF_FLAG_DEFAULT, nullptr, &wcharPath))) {
		std::filesystem::path home(wcharPath);
		CoTaskMemFree(wcharPath);
		return home;
	}

	// fallback to environment variables if API fails
	auto userProfile = std::getenv("USERPROFILE");

	if (userProfile) {
		return std::filesystem::path(userProfile);
	}

	auto homeDrive = std::getenv("HOMEDRIVE");
	auto homePath = std::getenv("HOMEPATH");

	if (homeDrive && homePath) {
		return std::filesystem::path(std::string(homeDrive) + std::string(homePath));
	}

#else
	auto home = std::getenv("HOME");

	if (home) {
		return std::filesystem::path(home);
	}

	// fallback: read the password database record for the current UID
	struct passwd* pw = getpwuid(geteuid());

	if (pw && pw->pw_dir) {
		return std::filesystem::path(pw->pw_dir);
	}

#endif

	return std::filesystem::path();
}


//
//	FileSelector::isHidden
//

bool FileSelector::isHidden(const std::filesystem::path& path) {
	if (path.empty()) {
		return true;
	}

#ifdef _WIN32
	auto dwAttr = GetFileAttributesW(path.c_str());
	return (dwAttr == INVALID_FILE_ATTRIBUTES) ? false : ((dwAttr & FILE_ATTRIBUTE_HIDDEN) != 0);

#else
	auto name = pathToString(path.filename());

	if (!name.empty() && name[0] == '.') {
		return true;
	}

#if __APPLE__
	struct stat info;

	if (stat(name.c_str(), &info) == 0) {
		if (info.st_flags & UF_HIDDEN) {
			return true;
		}
	}
#endif

	return false;
#endif
}


//
//	FileSelector::isAccessible
//

bool FileSelector::isAccessible(const std::filesystem::path& path) {
	// check if path exists and is a directory
	std::error_code ec;

	if (!std::filesystem::exists(path, ec) || !std::filesystem::is_directory(path, ec)) {
		return false;
	}

	// try iterating through the directory to confirm read access
	std::filesystem::directory_iterator it(path, ec);

	if (ec) {
		return false;
	}

	return true;
}


//
//	FileSelector::getKnownDirectoryInfo
//

void FileSelector::getKnownDirectoryInfo(KnownDirectory type, std::string& name, std::filesystem::path& path) {
#if __APPLE__
	// set default values
	static const char* types[] = {
		"Desktop",
		"Documents",
		"Downloads",
		"Movies",
		"Music",
		"Pictures"
	};

	name = types[static_cast<size_t>(type)];
	std::filesystem::path home(std::getenv("HOME"));
	path = home / name;

	// use CoreFoundation to find localized display name
	CFStringRef cfPath = CFStringCreateWithCString(kCFAllocatorDefault, path.c_str(), kCFStringEncodingUTF8);
	CFURLRef cfURL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, cfPath, kCFURLPOSIXPathStyle, true);
	CFStringRef localizedNameRef = nullptr;

	if (CFURLCopyResourcePropertyForKey(cfURL, kCFURLLocalizedNameKey, &localizedNameRef, nullptr)) {
		CFIndex length = CFStringGetLength(localizedNameRef);
		CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;

		std::vector<char> buffer(maxSize);
		CFStringGetCString(localizedNameRef, buffer.data(), maxSize, kCFStringEncodingUTF8);
		name = std::string(buffer.data());

		CFRelease(localizedNameRef);
	}

	CFRelease(cfURL);
	CFRelease(cfPath);

#elif _WIN32
	// set default values
	static const char* types[] = {
		"Desktop",
		"Documents",
		"Downloads",
		"Videos",
		"Music",
		"Pictures"
	};

	name = types[static_cast<size_t>(type)];
	auto home = getHome();
	path = home / name;

	// initialize COM library
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	static const KNOWNFOLDERID IDs[] = {
		FOLDERID_Desktop,
		FOLDERID_Documents,
		FOLDERID_Downloads,
		FOLDERID_Videos,
		FOLDERID_Music,
		FOLDERID_Pictures
	};

	// retrieve the known folder path
	PWSTR folderPath = NULL;

	if (SUCCEEDED(SHGetKnownFolderPath(IDs[static_cast<size_t>(type)], 0, NULL, &folderPath))) {
		path = folderPath;
		CoTaskMemFree(folderPath);
	}

	// uninitialize COM library
	CoUninitialize();

#else
	// set default values
	static const char* types[] = {
		"Desktop",
		"Documents",
		"Downloads",
		"Videos",
		"Music",
		"Pictures"
	};

	name = types[static_cast<size_t>(type)];
	auto home = getHome();
	path = home / name;

	// locate the XDG user-dirs config file
	const char* configHome = std::getenv("XDG_CONFIG_HOME");

	std::filesystem::path configFile = configHome
		? std::filesystem::path(configHome) / "user-dirs.dirs"
		: home / ".config" / "user-dirs.dirs";

	// search the config file for the requested key
	std::ifstream file(configFile);

	if (file.is_open()) {
		static const char* keys[] = {
			"XDG_DESKTOP_DIR",
			"XDG_DOCUMENTS_DIR",
			"XDG_DOWNLOAD_DIR",
			"XDG_VIDEOS_DIR",
			"XDG_MUSIC_DIR",
			"XDG_PICTURES_DIR"
		};

		std::string key = std::string("XDG_") + keys[static_cast<size_t>(type)] + "_DIR=";
		std::string line;

		while (std::getline(file, line)) {
			// look for the specific video/movie folder variable
			if (line.rfind(key) == 0) {
				// extract the path inside the quotes
				size_t firstQuote = line.find('"');
				size_t lastQuote = line.rfind('"');

				if (firstQuote != std::string::npos &&
					lastQuote != std::string::npos &&
					lastQuote > firstQuote) {

					// replace "$HOME" placeholder with the actual home directory path (if required)
					std::string rawPath = line.substr(firstQuote + 1, lastQuote - firstQuote - 1);
					path = (rawPath.rfind("$HOME", 0) == 0) ? (home / rawPath.substr(5)) : std::filesystem::path(rawPath);
					name = pathToString(path.filename());
				}
			}
		}
	}
#endif
}


//
//	FileSelector::forEachKnownLocation
//

void FileSelector::forEachKnownLocation(std::function<void(const std::string& name, const std::filesystem::path& path)> callback) {
#if __APPLE__
	std::filesystem::path volumes{"/Volumes"};

	for (const auto& entry : std::filesystem::directory_iterator(volumes)) {
		auto filename = pathToString(entry.path().filename());
		auto path = std::filesystem::canonical(entry.path());

		if (filename[0] != '.' && filename.rfind("com.", 0) != 0) {
			callback(filename, path);
		}
	}

#elif defined(_WIN32)
	// get list of logical drives
	DWORD bufferLength = GetLogicalDriveStringsW(0, nullptr);
	std::vector<wchar_t> buffer(bufferLength);
	GetLogicalDriveStringsW(bufferLength, buffer.data());

	// parse the null-separated block of strings
	std::vector<wchar_t> buffer2(MAX_PATH + 1);

	for (auto drive = buffer.data(); *drive; drive += wcslen(drive) + 1) {
		// get volume name
		std::string volumeName;
		GetVolumeInformationW(drive, buffer2.data(), MAX_PATH + 1, nullptr, nullptr, nullptr, nullptr, 0);
		auto size = static_cast<int>(wcslen(buffer2.data()));

		if (size) {
			auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, buffer2.data(), size, nullptr, 0, nullptr, nullptr);

			if (sizeNeeded) {
				volumeName = std::string(sizeNeeded, 0);
				WideCharToMultiByte(CP_UTF8, 0, buffer2.data(), size, volumeName.data(), sizeNeeded, nullptr, nullptr);
			}
		}

		// convert drive to path and logical name
		std::filesystem::path path{drive};
		auto name = pathToString(path.root_name()) + " " + volumeName;

		// add to list
		callback(name, path);
	}

#else
	// open the mounted filesystems table file
	auto file = setmntent("/proc/mounts", "r");

	if (file == nullptr) {
		return;
	}

	// iterate through each mount entry
	for (struct mntent* entry = getmntent(file); entry != nullptr; entry = getmntent(file)) {
		std::string mountPoint(entry->mnt_dir);

		// look for paths typically handled by user space managers
		if (mountPoint.rfind("/media/", 0) == 0 || mountPoint.rfind("/run/media/", 0) == 0) {
			callback(entry->mnt_fsname, entry->mnt_dir);
		}
	}

	endmntent(file);
#endif
}


//
//	FileSelector::movePathToTrashCan
//

bool FileSelector::movePathToTrashCan(const std::filesystem::path& path) {
	// determine absolute path with .. and symbolic links resolved
	auto canonicalPath = std::filesystem::canonical(path);

#if __APPLE__
	auto canonicalString = pathToString(canonicalPath);

	void* pool = objc_autoreleasePoolPush();

	Class NSStringClass = objc_getClass("NSString");
	SEL stringWithUTF8StringSel = sel_registerName("stringWithUTF8String:");
	id pathString = ((id(*)(Class, SEL, const char*)) objc_msgSend)(NSStringClass, stringWithUTF8StringSel, canonicalString.c_str());

	Class NSFileManagerClass = objc_getClass("NSFileManager");
	SEL defaultManagerSel = sel_registerName("defaultManager");
	id fileManager = ((id(*)(Class, SEL)) objc_msgSend)(NSFileManagerClass, defaultManagerSel);

	Class NSURLClass = objc_getClass("NSURL");
	SEL fileURLWithPathSel = sel_registerName("fileURLWithPath:");
	id nsurl = ((id(*)(Class, SEL, id)) objc_msgSend)(NSURLClass, fileURLWithPathSel, pathString);

	SEL trashItemAtURLSel = sel_registerName("trashItemAtURL:resultingItemURL:error:");
	auto result = ((BOOL(*)(id, SEL, id, id, id)) objc_msgSend)(fileManager, trashItemAtURLSel, nsurl, nil, nil);

	objc_autoreleasePoolPop(pool);
	return result;

#elif _WIN32
	// double terminate string for WIN32 API
	auto canonicalString = canonicalPath.generic_wstring();
	canonicalString.push_back(L'\0');
	canonicalString.push_back(L'\0');

	// run native API
	SHFILEOPSTRUCTW fileOp = {0};
	fileOp.wFunc = FO_DELETE;
	fileOp.pFrom = canonicalString.data();
	fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;
	return SHFileOperationW(&fileOp) == 0;

#else
	// use desktop command to move files to trash
	std::string command = "gio trash '" + pathToString(canonicalPath) + "'";
	return std::system(command.c_str()) == 0;
#endif
}
