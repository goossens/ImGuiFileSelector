//	FileSelector - A file select dialog for Dear ImGui.
//	Copyright (c) 2026 Johan A. Goossens. All rights reserved.
//
//	This work is licensed under the terms of the MIT license.
//	For a copy, see <https://opensource.org/licenses/MIT>.


#pragma once


//
//	Include files
//

#include <filesystem>
#include <string>
#include <vector>

#include "imgui.h"


//
//	FileSelector
//
//	This class implements a file selector dialog window for Dear ImGui. The layout
//	for this dialog is inspired by MacOS but the look and feel is pure Dear ImGui.
//	For now, the visualization is limited to the List view.
//

class FileSelector {
public:
	// singleton access
	static inline FileSelector& Instance() {
		static FileSelector singleton;
		return singleton;
	}

	// constructor
	FileSelector();

	//	access options
	inline void SetShowSideBar(bool value) { showSideBar = value; }
	inline bool GetShowSideBar() const { return showSideBar; }
	inline void SetShowHidden(bool value) { showHidden = value; }
	inline bool GetShowHidden() const { return showHidden; }

	// access state
	inline bool SetCurrentPath(const std::filesystem::path& path) { return setCurrentPath(path, false); }
	inline const std::filesystem::path& GetCurrentPath() const { return state.currentPath; }

	// start a selector to open a single file
	// returns true if selector is opened and false if a previous selector is still active
	bool OpenFile(const std::string& filter="*");

	// start a selector to pick a path to save content to
	// returns true if selector is opened and false if a previous selector is still active
	bool SaveAs();

	// start a selector to select one or more files
	// returns true if selector is opened and false if a previous selector is still active
	bool SelectFiles(const std::string& filter="*");

	// start a selector to select a directory
	// returns true if selector is opened and false if a previous selector is still active
	bool SelectDirectory();

	// forcefully close the current selector
	// this doesn't do anything if no selector is open
	//
	// it also doesn't need to be called when a selector completes as that is handled internally
	inline void Close() { type = Type::idle; }

	// check currect selector state
	inline bool IsOpen() const { return type != Type::idle; }
	inline bool IsOpenFileOpen() const { return type != Type::openFile; }
	inline bool IsSaveAsOpen() const { return type != Type::saveAs; }
	inline bool IsSelectFilesOpen() const { return type != Type::selectFiles; }
	inline bool IsSelectDirectoryOpen() const { return type != Type::selectDirectory; }

	// get selection status
	inline bool WasCancelled() const { return action == Action::cancelled; }
	inline bool SelectedOpenFile() const { return action == Action::selectedOpenFile; }
	inline bool SelectedSaveAs() const { return action == Action::selectedSaveAs; }
	inline bool SelectedFiles() const { return action == Action::selecteFiles; }
	inline bool SelectedDirectory() const { return action == Action::selectedDirectory; }

	inline const std::filesystem::path& GetSelectedPath() const { return selectedPath; }

	// render the file selector widget
	// it is safe to call this every frame as it's a NOOP if no selectors are active
	// returns true if user made selection or false if not
	bool Render();

	// manage sidebar content
	inline void ClearFavorites() { favorites.entries.clear(); }
	inline void AddFavorite(const std::string& name, const std::filesystem::path& path) { favorites.entries.emplace_back(name, path); }
	inline void AddDefaultFavorites() { addDefaultFavorites(); }

	inline void ClearClouds() { clouds.entries.clear(); }
	inline void AddCloud(const std::string& name, const std::filesystem::path& path) { clouds.entries.emplace_back(name, path); }
	inline void AddDefaultClouds() { addDefaultClouds(); }

	inline void ClearLocations() { locations.entries.clear(); }
	inline void AddLocation(const std::string& name, const std::filesystem::path& path) { locations.entries.emplace_back(name, path); }
	inline void AddDefaultLocations() { addDefaultLocations(); }

	inline void ClearMedia() { media.entries.clear(); }
	inline void AddMedia(const std::string& name, const std::filesystem::path& path) { media.entries.emplace_back(name, path); }
	inline void AddDefaultMedia() { addDefaultMedia(); }

	// state access
	struct State {
		std::filesystem::path currentPath;
		std::vector<std::filesystem::path> recentPlaces;
		size_t sortColumn = 0;
		bool sortAscending = true;
	};

	const State& GetCurrentState() const { return state; }
	void RestoreState(const State& newState) { state = newState; }

	// internationalization support
	struct Labels {
		std::string ok;
		std::string cancel;
		std::string nameColumn;
		std::string dateColumn;
		std::string sizeColumn;
		std::string search;
		std::string favorites;
		std::string clouds;
		std::string locations;
		std::string media;
		std::string newFolder;
		std::string recentPlaces;
		std::string confirmationWindow;
		std::string errorWindow;
		std::string cantAccess;
		std::string rename;
		std::string duplicate;
		std::string moveToTrash;
		std::string timeFormat;
		std::string bytes;
		std::string kiloBytes;
		std::string megaBytes;
		std::string gigaBytes;
		std::string terraBytes;
		std::string petaBytes;
		std::string exoBytes;
	};

	inline void SetLabels(struct Labels& newLabels) { labels = newLabels; }
	inline const Labels& GetLabels() const { return labels; }

private:
#if __cplusplus >= 202002L
		using PathString = std::u8string;

	#else
		using PathString = std::string;
#endif

	// configuration
	bool showSideBar = true;
	bool showHidden = false;

	Labels labels = {
		"OK",
		"Cancel",
		"Name",
		"Date",
		"Size",
		"search...",
		"Favorites",
		"Clouds",
		"Locations",
		"Media",
		"New Folder",
		"Recent Places",
		"Confirmation...",
		"Error...",
		"Can't access",
		"Rename",
		"Move to Trash",
		"Duplicate",
		"%b %d, %Y at %I:%M %p",
		"B ",
		"KB",
		"MB",
		"GB",
		"TB",
		"PB",
		"EB"
	};

	// current dialog type
	enum class Type {
		idle,
		openFile,
		saveAs,
		selectFiles,
		selectDirectory
	} type = Type::idle;

	// last action taken
	enum class Action {
		none,
		cancelled,
		selectedOpenFile,
		selectedSaveAs,
		selecteFiles,
		selectedDirectory
	}  action = Action::none;

	// current state
	State state;
	std::filesystem::path selectedPath;
	bool isOpen;

	// directory traversal history in current session
	std::vector<std::filesystem::path> pathHistory;
	size_t historyIndex = 0;

	// a named path
	struct NamedPath {
		NamedPath() = default;
		NamedPath(PathString name, std::filesystem::path path) : name(name), path(path) {}
		PathString name;
		std::filesystem::path path;
	};

	// parts of the current path in reverse order
	std::vector<NamedPath> pathStack;

	// list of nodes (files and directories) at current path
	struct Node {
		std::filesystem::path path;
		bool isDirectory;
		std::uintmax_t size;
		std::filesystem::file_time_type lastUpdate;
		bool isSelected;

		PathString pathString;
		std::string sizeString;
		std::string updateString;
		std::wstring sortString;

		std::string readableSize(const Labels& labels);
		std::string readableDate(const Labels& labels);
	};

	std::vector<Node> nodes;
	std::filesystem::file_time_type lastDirectoryWriteTime;

	// sidebar groups
	struct SideBarGroup {
		std::vector<NamedPath> entries;
		bool expanded = true;

		inline void add(const std::string& name, const std::filesystem::path& path) {
			if (std::filesystem::exists(path)) {
				entries.emplace_back(name, path);
			}
		}
	};

	SideBarGroup favorites;
	SideBarGroup clouds;
	SideBarGroup locations;
	SideBarGroup media;

	// error handling
	std::string errorMessage;
	std::string errorDetails;
	bool openErrorMessage = false;

	// work variables
	float frameHeight;
	ImVec2 glyphSize;
	ImVec2 itemSpacing;

	// local functions
	bool openDialog(Type type);
	bool setCurrentPath(const std::filesystem::path path, bool addHistory=true);
	bool refreshNodes(const std::filesystem::path& path);
	void sortNodes();

	void clearSelections();

	void renderFileDialog();
	void renderSideBar();
	void renderSideBarGroup(const std::string& label, SideBarGroup& group);
	void renderHeader();
	void renderListView(ImVec2 size);
	void renderActionButtons();
	void renderPopups();
	bool grouping(const char* label, bool* expanded);
	void spacing();

	void addDefaultFavorites();
	void addDefaultClouds();
	void addDefaultLocations();
	void addDefaultMedia();

	bool isHidden(const std::filesystem::path & path);

	inline void setErrorMessage(const std::string& message, const std::string& details="") {
		errorMessage = message;
		errorDetails = details;
		openErrorMessage = true;
	}

	std::filesystem::path getHome();
};
