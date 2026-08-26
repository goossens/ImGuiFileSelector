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
	// set current path to OS working directory
	setCurrentPath(std::filesystem::current_path(), false);

	// add default sidebar links
	addDefaultFavorites();
	addDefaultClouds();
	addDefaultLocations();
	addDefaultMedia();
}


//
//	FileSelector::OpenFile
//

bool FileSelector::OpenFile(const std::string&) {
	return openDialog(Type::openFile);
}


//
//	FileSelector::SaveAs
//

bool FileSelector::SaveAs() {
	return openDialog(Type::saveAs);
}


//
//	FileSelector::SelectFiles
//

bool FileSelector::SelectFiles(const std::string&) {
	return openDialog(Type::selectFiles);
}


//
//	FileSelector::SelectDirectory
//

bool FileSelector::SelectDirectory() {
	return openDialog(Type::selectDirectory);
}


//
//	FileSelector::Render
//

bool FileSelector::Render() {
	// don't do anything if a file selector isn't open yet
	if (type == Type::idle) {
		return false;
	}

	// ask popup to be opened (if required)
	if (!isOpen) {
		ImGui::OpenPopup("ImGuiFileSelector");
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
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoScrollbar;

	if (ImGui::BeginPopupModal("###ImGuiFileSelector", nullptr, windowFlags)) {
		action = Action::none;
		renderFileDialog();

		// see if user performed action (selection or cancel)
		if (action != Action::none) {
			// handle file open mode
			if (action == Action::selectedOpenFile) {
				// user selected a file
				// get directory of selected file
				auto directory = selectedPath.parent_path();

				// remove old recent places entry to avoid duplicates
				auto i = std::find(state.recentPlaces.begin(), state.recentPlaces.end(), directory);

				if (i != state.recentPlaces.end()) {
					state.recentPlaces.erase(i);
				}

				// limit list (if required) and add new entry
				if (state.recentPlaces.size() > 7) { state.recentPlaces.resize(7); }
				state.recentPlaces.emplace(state.recentPlaces.begin(), directory);
			}

			// close selector popup
			ImGui::CloseCurrentPopup();
			type = Type::idle;
			isOpen = false;
		}

		// handle possible popups
		renderPopups();
		ImGui::EndPopup();
	}

	return action != Action::none;
}


//
//	FileSelector::openDialog
//

bool FileSelector::openDialog(Type openType) {
	if (type == Type::idle) {
		refreshNodes(state.currentPath);
		sortNodes();

		type = openType;
		selectedPath.clear();
		isOpen = false;
		clearSelections();

		pathHistory.clear();
		pathHistory.emplace_back(state.currentPath);
		historyIndex = 0;
		return true;

	} else {
		return false;
	}
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
	sortNodes();

	// save new path
	state.currentPath = canonicalPath;

	// build a stack of path parts (in reverse order)
	pathStack.clear();
	std::filesystem::path partialPath;

	for (auto i = state.currentPath.begin(); i != state.currentPath.end(); i++) {
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
				if (showHidden || !isHidden(entry.path())) {
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

void FileSelector::sortNodes() {
	// sort current nodes
	std::sort(nodes.begin(), nodes.end(), [this](const Node& left, const Node& right) {
		if (state.sortColumn == 0) {
			return (state.sortAscending)
				? left.sortString < right.sortString
				: left.sortString > right.sortString;

		} else if (state.sortColumn == 1) {
			return (state.sortAscending)
				? left.lastUpdate < right.lastUpdate
				: left.lastUpdate > right.lastUpdate;

		} else if (state.sortColumn == 2) {
			return (state.sortAscending)
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

	if (showSideBar) {
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
		header(label.c_str(), &(group.visible));

		if (group.visible) {
			ImGui::Indent();

			for (auto& entry : group.entries) {
				ImGui::PushID(&entry);

				if (ImGui::Selectable(reinterpret_cast<const char*>(entry.name.c_str()))) {
					setCurrentPath(entry.path);
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

		for (auto i = state.recentPlaces.begin(); i < state.recentPlaces.end(); i++) {
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
				sortSpecs->SpecsDirty = false;
				state.sortColumn = static_cast<size_t>(sortSpecs->Specs->ColumnIndex);
				state.sortAscending	= sortSpecs->Specs->SortDirection == ImGuiSortDirection_Ascending;
				sortNodes();
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
						action = Action::selectedOpenFile;
					}

				} else {
					clearSelections();
					node.isSelected = true;
					selectedPath = node.path;
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

	// add ability to create a new folder (if required)
	if (type == Type::saveAs) {
		if (ImGui::Button(labels.newFolder.c_str())) {
		}

		ImGui::SameLine();
	}

	// right align buttons
	auto availableSpace = ImGui::GetContentRegionAvail();
	auto size = ImVec2((std::max(labels.ok.size(), labels.cancel.size()) + 2) * glyphSize.x, 0.0f);
	auto pos = ImGui::GetCursorScreenPos();

	ImGui::SetCursorScreenPos(ImVec2(pos.x + availableSpace.x - size.x * 2.0f - itemSpacing.x, pos.y));

	// handle cancel button and shortcut
	if (ImGui::Button(labels.cancel.c_str(), size) ||ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteAlways)) {
		selectedPath.clear();
		action = Action::cancelled;
	}

	// handle OK button (disable when nothing is selected)
	ImGui::SameLine();

	if (selectedPath.empty()) {
		ImGui::BeginDisabled();
	}

	if (ImGui::Button(labels.ok.c_str(), size)) {
		switch (type) {
			case Type::openFile: action = Action::selectedOpenFile; break;
			case Type::saveAs: action = Action::selectedSaveAs; break;
			case Type::selectFiles: action = Action::selecteFiles; break;
			case Type::selectDirectory: action = Action::selectedDirectory; break;
			default: break;
		}
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
//	FileSelector::header
//

bool FileSelector::header(const char* label, bool* state) {
	// determine position and space
	auto pos = ImGui::GetCursorScreenPos();
	auto size = ImGui::GetContentRegionAvail();
	size.y = glyphSize.y;

	// run button action
	bool changed = ImGui::InvisibleButton(label, size);

	if (changed) {
		*state = !*state;
	}

	// render label and state
	auto drawList = ImGui::GetWindowDrawList();
	auto color = ImGui::GetColorU32(ImGuiCol_TextDisabled);
	drawList->AddText(pos, color, label);

	if (ImGui::IsItemHovered()) {
		auto right = pos + ImVec2(size.x - glyphSize.x, 0.0f);
		ImVec2 p1 = ImVec2(right + ImVec2(0.0f, glyphSize.y * 0.3f));
		ImVec2 p2 = right + (*state ? ImVec2(glyphSize.x * 0.5f, glyphSize.y * 0.7f) : ImVec2(glyphSize.x, glyphSize.y * 0.5f));
		ImVec2 p3 = right + (*state ? ImVec2(glyphSize.x, glyphSize.y * 0.3f) : ImVec2(0.0f, glyphSize.y * 0.7f));
		drawList->AddLine(p1, p2, color);
		drawList->AddLine(p2, p3, color);
	}

	// run result
	return changed;
}


//
//	FileSelector::spacing
//

void FileSelector::spacing() {
	auto pos = ImGui::GetCursorScreenPos();
	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + frameHeight * 0.4f));
}


//
//	FileSelector::addDefaultFavorites
//

void FileSelector::addDefaultFavorites() {
	auto home = getHome();

	if (!home.empty()) {
		// these only get added when they exist
		favorites.add("Home", home);
		favorites.add("Desktop", home / "Desktop");
		favorites.add("Documents", home / "Documents");
		favorites.add("Downloads", home / "Downloads");
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
//	FileSelector::addDefaultMedia
//

void FileSelector::addDefaultMedia() {
	auto home = getHome();

	if (!home.empty()) {
		media.add("Movies", home / "Movies");
		media.add("Music", home / "Music");
		media.add("Pictures", home / "Pictures");
	}
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
//	Operating System specific functions
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
#include <shlobj.h>

#else
#include <pwd.h>
#include <unistd.h>

#ifndef __APPLE__
#include <mntent.h>
#endif

#endif


//
//	FileSelector::addDefaultLocations
//

void FileSelector::addDefaultLocations() {
#if __APPLE__
	std::filesystem::path volumes{"/Volumes"};

	for (const auto& entry : std::filesystem::directory_iterator(volumes)) {
		auto path = entry.path();
		locations.add(path.filename(), std::filesystem::canonical(path));
	}

#elif defined(_WIN32)
	// get list of logical drives
	DWORD bufferLength = GetLogicalDriveStringsW(0, nullptr);
	std::vector<wchar_t> buffer(bufferLength);
	GetLogicalDriveStringsW(bufferLength, buffer.data());

	// parse the null-separated block of strings
	for (auto drive = buffer.data(); *drive; drive += wcslen(drive) + 1) {
		// convert drive to path and logical name
		std::filesystem::path path{drive};
		auto name = path.u8string();

		// add drive type (if possible)
		switch (GetDriveTypeW(drive)) {
			case DRIVE_REMOVABLE: name += " (Removable)"; break;
			case DRIVE_FIXED: name += " (Fixed)"; break;
			case DRIVE_REMOTE: name += " (Network)"; break;
			case DRIVE_CDROM: name += " (CD-ROM)"; break;
			case DRIVE_RAMDISK: name += " (RAM)"; break;
			default: break;
		}

		// add to list
		locations.add(name, path);
	}

#else
	// open the mounted filesystems table file
	auto file = setmntent("/proc/mounts", "r");

	if (file == nullptr) {
		return;
	}

	// iterate through each mount entry
	while (struct mntent* entry = getmntent(file); entry != nullptr; entry = getmntent(file)) {
		// filter out pseudo-filesystems to get actual drives
		if (entry->mnt_fsname[0] == '/') {
			locations.add(entry->mnt_fsname, entry->mnt_dir);
		}
	}

	endmntent(file);

#endif
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
	PathString name = path.filename().u8string();
    return name[0] == '.' && name != "." && name != "..";
#endif
}


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
