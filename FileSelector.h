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
//	For now, the visualization is limited to List view.
//

class FileSelector {
public:
	//
	// singleton access
	static inline FileSelector& Instance() {
		static FileSelector singleton;
		return singleton;
	}

	// constructor
	FileSelector();

	//	access options
	inline bool SetCurrentPath(const std::filesystem::path& path) { return setCurrentPath(path, false); }
	inline const std::filesystem::path& GetCurrentPath() const { return currentPath; }
	inline void SetShowHiddenNodes(bool value) { showHiddenNodes = value; }
	inline bool GetShowHiddenNodes() const { return showHiddenNodes; }

	// start a file selector to open a single file
	// returns true if selector is opened and false if a previous file selector is still active
	bool OpenFile(const char* label, const std::string& filter="*");

	// start a file selector to pick a path to save a file to
	// returns true if selector is opened and false if a previous file selector is still active
	bool SaveAs(const char* label);

	// start a file selector to select one or more files
	// returns true if selector is opened and false if a previous file selector is still active
	bool SelectFiles(const char* label, const std::string& filter="*");

	// start a file selector to select a directory
	// returns true if selector is opened and false if a previous file selector is still active
	bool SelectDirectory(const char* label);

	// see if specified selector is open
	// inline bool IsOpen(const char* id) const { return currentID == id; }

	// render
	bool Render(ImVec2 size=ImVec2(800, 400));

	// internationalization support
	struct Labels {
		std::string ok;
		std::string cancel;
		std::string nameColumn;
		std::string dateColumn;
		std::string sizeColumn;
		std::string timeFormat;
	};

	inline void SetLabels(struct Labels& newLabels) { labels = newLabels; }
	inline const Labels& GetLabels() const { return labels; }

private:
	// configuration
	bool showHiddenNodes = false;
	std::filesystem::path currentPath;

	Labels labels = {
		"OK",
		"Cancel",
		"Name",
		"Date",
		"Size",
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
	std::string currentLabel;
	bool isOpen;
	bool hasAction;

	ImS16 sortColumn = 0;
	ImGuiSortDirection sortDirection = ImGuiSortDirection_Ascending;

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

	// list of nodes (files and directories) at current path
	struct Node {
		std::filesystem::path path;
		bool isDirectory;
		std::uintmax_t size;
		std::filesystem::file_time_type lastUpdate;
		bool isSelected;

		std::string pathString;
		std::string sizeString;
		std::string updateString;

		std::string readableSize();
		std::string readableDate(const std::string& format);
	};

	std::vector<Node> nodes;

	// work variables
	float frameHeight;
	ImVec2 glyphSize;
	ImVec2 itemSpacing;

	// local functions
	bool setCurrentPath(const std::filesystem::path& path, bool addHistory=true);
	void sortNodes(ImS16 column, ImGuiSortDirection direction);
	void clearSelections();
	void renderFileDialog();
	void renderSideBar();
	void renderHeader();
	void renderListView(ImVec2 size);
	void renderActionButtons();
	void spacing();
	bool isHidden(const std::filesystem::path & path);
};
