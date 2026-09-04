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
#include <string>
#include <vector>


//
//	Entry
//

struct Entry {
	// informating about a single directory entry
	std::filesystem::path path;
	bool isDirectory;
	std::uintmax_t size;
	std::string extension;
	std::filesystem::file_time_type lastUpdate;
	bool isSelected;

	std::string nameString;
	std::string sizeString;
	std::string updateString;
	std::wstring sortString;

	std::string readableSize();
	std::string readableDate(const std::string& format);
};


//
//	Listing
//

class Listing : public std::vector<Entry> {
public:
	// load a specified path
	bool load(const std::filesystem::path& path, const std::string& timeFormat);

	// set filter parameters
	inline void setShowHidden(bool show) { showHidden = show; }
	void setSort(size_t column, bool ascending);
	void setExtensionFilter(const std::string& filter);
	inline void setSearch(const std::string& search) { searchCriteria = search; }

	// iterate through listing
	void forEach(std::function<void(const Entry&)> callback);

private:
	// properties
	std::filesystem::path currentPath;
	std::filesystem::file_time_type lastWriteTime;
	size_t sortColumn = 0;
	bool sortAscending = true;
	bool showHidden = false;
	std::vector<std::string> extensionFilter;
	std::string searchCriteria;
	std::string error;

	// support functions
	void sort();
	bool filter(const Entry& entry);
};
