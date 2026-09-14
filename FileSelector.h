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
#include <functional>
#include <regex>
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
	// singleton implementation
	static inline FileSelector& Instance() {
		static FileSelector singleton;
		return singleton;
	}

	// constructor
	FileSelector();

	// sort options
	enum class SortColumn {
		name,
		date,
		size
	};

	enum class SortOrder
	 {
		ascending,
		descending
	};

	//	access options/state
	inline void SetShowSideBar(bool value) { state.showSideBar = value; }
	inline bool GetShowSideBar() const { return state.showSideBar; }
	inline void SetShowHidden(bool value) { state.showHidden = value; }
	inline bool GetShowHidden() const { return state.showHidden; }
	inline void SetSortColumn(SortColumn value) { state.sortColumn = value; }
	inline SortColumn GetSortColumn() const { return state.sortColumn; }
	inline void SetSortOrder(SortOrder value) { state.sortOrder = value; }
	inline SortOrder GetSortOrder() const { return state.sortOrder; }

	inline bool SetCurrentPath(const std::filesystem::path& path) { return setCurrentPath(path, false); }
	inline const std::filesystem::path& GetCurrentPath() const { return state.currentPath; }

	// start a selector to open a single file
	// returns true if selector is opened and false if a previous selector is still active
	bool OpenFile(const std::string& filter="");

	// start a selector to pick a path to save content to
	// returns true if selector is opened and false if a previous selector is still active
	bool SaveAs();

	// start a selector to select one or more files
	// returns true if selector is opened and false if a previous selector is still active
	bool SelectFiles(const std::string& filter="");

	// start a selector to select a directory
	// returns true if selector is opened and false if a previous selector is still active
	bool SelectDirectory(const std::string& filter="");

	// forcefully close the current selector
	// this doesn't do anything if no selector is open
	//
	// it also doesn't need to be called when a selector completes as that is handled internally
	inline void Close() { mode = Mode::idle; }

	// check currect selector state
	inline bool IsOpen() const { return mode != Mode::idle; }
	inline bool IsOpenFileOpen() const { return mode != Mode::openFile; }
	inline bool IsSaveAsOpen() const { return mode != Mode::saveAs; }
	inline bool IsSelectFilesOpen() const { return mode != Mode::selectFiles; }
	inline bool IsSelectDirectoryOpen() const { return mode != Mode::selectDirectory; }

	// get selection status
	inline bool WasCancelled() const { return action == Action::cancelled; }
	inline bool SelectedOpenFile() const { return action == Action::selectedOpenFile; }
	inline bool SelectedSaveAs() const { return action == Action::selectedSaveAs; }
	inline bool SelectedFiles() const { return action == Action::selectedFiles; }
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
		bool showSideBar = true;
		bool showHidden = false;
		SortColumn sortColumn = SortColumn::name;
		SortOrder sortOrder = SortOrder::ascending;
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
		std::string filter;
		std::string favorites;
		std::string clouds;
		std::string locations;
		std::string media;
		std::string saveAs;
		std::string newFolder;
		std::string recentPlaces;
		std::string confirmationWindow;
		std::string errorWindow;
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
	// configuration
	Labels labels = {
		"OK",
		"Cancel",
		"Name",
		"Date",
		"Size",
		"filter...",
		"Favorites",
		"Clouds",
		"Locations",
		"Media",
		"Save As:",
		"New Folder",
		"Recent Places",
		"Confirmation...",
		"Error...",
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

	// current dialog mode
	enum class Mode {
		idle,
		openFile,
		saveAs,
		selectFiles,
		selectDirectory
	} mode = Mode::idle;

	// last action taken
	enum class Action {
		none,
		cancelled,
		selectedOpenFile,
		selectedSaveAs,
		selectedFiles,
		selectedDirectory
	}  action = Action::none;

	// current state
	State state;
	std::filesystem::path selectedPath;
	std::vector<std::filesystem::path> selectedPaths;
	bool requestOpen = false;

	// directory traversal history in current session
	std::vector<std::filesystem::path> pathHistory;
	size_t historyIndex = 0;

	// a named path
	struct NamedPath {
		NamedPath() = default;
		NamedPath(std::string name, std::filesystem::path path) : name(name), path(path) {}
		std::string name;
		std::filesystem::path path;
	};

	// parts of the current path in reverse order
	std::vector<NamedPath> pathStack;

	// single directory entry
	struct Entry {
		// informating about a single directory entry
		std::filesystem::path path;
		bool isDirectory;
		std::uintmax_t size;
		std::string extension;
		std::filesystem::file_time_type lastUpdate;
		bool isSelected;
		bool isHidden;

		std::string nameString;
		std::string sizeString;
		std::string updateString;
		std::wstring sortString;

		std::string readableSize(const Labels& labels);
		std::string readableDate(const Labels& labels);
	};

	// current directory listing
	class Listing : public std::vector<Entry> {
	public:
		// load a specified path
		bool load(const std::filesystem::path& path, const Labels& labels);

		// set filter parameters
		inline void setShowHidden(bool show) { showHidden = show; }
		void setSort(SortColumn column, SortOrder order);
		void setExtensionFilter(const std::string& filter);
		void setUserFilter(const std::string& filter);

		// iterate through listing
		void forEach(std::function<void(Entry&)> callback);

		// clear all selections
		void clearSelections();

	private:
		// properties
		std::filesystem::path currentPath;
		std::filesystem::file_time_type lastWriteTime;
		SortColumn sortColumn = SortColumn::name;
		SortOrder sortOrder = SortOrder::ascending;
		bool showHidden = false;
		std::vector<std::string> extensions;
		bool filterActive = false;
		std::regex filterRegex;
		std::string error;

		// support functions
		void sort();
		bool filter(const Entry& entry);
	} listing;

	// known user directory types
	enum class KnownDirectory {
		desktop,
		documents,
		downloads,

		movies,
		music,
		pictures
	};

	// sidebar groups
	struct SideBarGroup {
		std::vector<NamedPath> entries;
		bool expanded = true;

		inline void add(const std::string& name, const std::filesystem::path& path) {
			if (isAccessible(path)) {
				entries.emplace_back(name, path);
			}
		}

		inline void add(KnownDirectory directory) {
			std::string name;
			std::filesystem::path path;
			getKnownDirectoryInfo(directory, name, path);
			add(name, path);
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
	std::string saveAsString;
	std::string filterString;
	std::filesystem::path nextPath;

	// local functions
	bool openDialog(Mode mode, const std::string& filter="");
	bool setCurrentPath(const std::filesystem::path path, bool addHistory=true);

	void renderFileDialog();
	void renderSideBar();
	void renderSideBarGroup(const std::string& label, SideBarGroup& group);
	void renderHeader();
	void renderListView(ImVec2 size);
	void renderActionButtons();
	void renderPopups();

	void addDefaultFavorites();
	void addDefaultClouds();
	void addDefaultLocations();
	void addDefaultMedia();

	void handleEntrySelection(Entry& entry);
	bool isOkAvailable();
	void handleOk();

	void spacing();
	bool grouping(const char* label, bool* expanded);
	static bool inputString(const char* label, std::string* value);
	static bool inputStringWithHint(const char* label, const char* hint, std::string* value);

	static std::filesystem::path getHome();
	static bool isHidden(const std::filesystem::path& path);
	static bool isAccessible(const std::filesystem::path& path);
	static void getKnownDirectoryInfo(KnownDirectory type, std::string& label, std::filesystem::path& path);
	static void forEachKnownLocation(std::function<void(const std::string& name, const std::filesystem::path& path)> callback);
	static bool movePathToTrashCan(const std::filesystem::path& path);

#if (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L) || (__cplusplus >= 202002L)
	static inline std::string pathToString(const std::filesystem::path& path) {
		auto u8Str = path.generic_u8string();
		std::string str(u8Str.begin(), u8Str.end());
		return str;
	}

	#else
	static inline std::string pathToString(const std::filesystem::path& path) {
		return path.u8string();
	}
#endif
};
