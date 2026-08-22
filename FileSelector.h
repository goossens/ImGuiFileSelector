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
	inline void SetShowHiddenNodes(bool value) { showHiddenNodes = value; }
	inline bool GetShowHiddenNodes() const { return showHiddenNodes; }

	// access state
	inline bool SetCurrentPath(const std::filesystem::path& path) { return setCurrentPath(path, false); }
	inline const std::filesystem::path& GetCurrentPath() const { return currentPath; }

	// start a file selector to open a single file
	// returns true if selector is opened and false if a previous file selector is still active
	bool OpenFile(const std::string& filter="*");

	// start a file selector to pick a path to save a file to
	// returns true if selector is opened and false if a previous file selector is still active
	bool SaveAs();

	// start a file selector to select one or more files
	// returns true if selector is opened and false if a previous file selector is still active
	bool SelectFiles(const std::string& filter="*");

	// start a file selector to select a directory
	// returns true if selector is opened and false if a previous file selector is still active
	bool SelectDirectory();

	// see if selector is currently open
	inline bool IsOpen() const { return type != Type::idle; }

	// forcefully close the current selector
	// this doesn't do anything if no selector is open
	//
	// it also doesn't need to be called when a selector completes
	// as that is handle internally
	inline void Close() { type = Type::idle; }

	// render the file selector widget
	// it is safe to call this every frame as it's a NOOP if no selectors are active
	bool Render();

	// internationalization support
	struct Labels {
		std::string ok;
		std::string cancel;
		std::string nameColumn;
		std::string dateColumn;
		std::string sizeColumn;
		std::string search;
		std::string favorites;
		std::string locations;
		std::string newFolder;
		std::string recentPlaces;
		std::string confirmationWindow;
		std::string errorWindow;
		std::string cantAccess;
		std::string rename;
		std::string duplicate;
		std::string moveToTrash;
		std::string timeFormat;
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
	bool showHiddenNodes = false;
	std::filesystem::path currentPath;

	Labels labels = {
		"OK",
		"Cancel",
		"Name",
		"Date",
		"Size",
		"search...",
		"Favorites",
		"Locations",
		"New Folder",
		"Recent Places",
		"Confirmation...",
		"Error...",
		"Can't access",
		"Rename",
		"Move to Trash",
		"Duplicate",
		"%b %d, %Y at %I:%M %p"
	};

	// current state
	enum class Type {
		idle,
		openFile,
		saveAs,
		selectFiles,
		selectDirectory
	} type = Type::idle;

	std::filesystem::path selectedPath;
	bool isOpen;
	bool hasAction;

	ImS16 sortColumn = 0;
	ImGuiSortDirection sortDirection = ImGuiSortDirection_Ascending;

	// directory traversal history in current session
	std::vector<std::filesystem::path> pathHistory;
	size_t historyIndex = 0;

	// recent places
	std::vector<std::filesystem::path> recentPlaces;

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

		std::string readableSize();
		std::string readableDate(const std::string& format);
	};

	std::vector<Node> nodes;
	std::filesystem::file_time_type lastDirectoryWriteTime;

	// error handling
	std::string errorMessage;
	std::string errorDetails;
	bool openErrorMessage = false;

	// work variables
	float frameHeight;
	ImVec2 glyphSize;
	ImVec2 itemSpacing;

	// local functions
	bool setCurrentPath(const std::filesystem::path path, bool addHistory=true);
	bool refreshNodes(const std::filesystem::path& path);
	void sortNodes(ImS16 column, ImGuiSortDirection direction);

	void clearSelections();

	void renderFileDialog();
	void renderSideBar();
	void renderHeader();
	void renderListView(ImVec2 size);
	void renderActionButtons();
	void renderPopups();
	void spacing();

	bool isHidden(const std::filesystem::path & path);

	inline void setErrorMessage(const std::string& message, const std::string& details="") {
		errorMessage = message;
		errorDetails = details;
		openErrorMessage = true;
	}
};
