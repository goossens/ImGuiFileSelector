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

#include "listing.h"


//
//	Listing::load
//

bool Listing::load(const std::filesystem::path& path, const std::string& timeFormat) {
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
			if (node.is_regular_file() || node.is_directory()) {
				// create a new entry
				auto& entry = entries.emplace_back();

				// get entry metadata and set state
				entry.path = node.path();
				entry.isDirectory = node.is_directory();
				entry.size = node.is_regular_file() ? node.file_size() : 0;
				entry.extension = node.is_regular_file() ? entry.path.extension().string() : "";
				entry.lastUpdate = node.last_write_time();
				entry.isSelected = false;

				// precalculate strings for faster rendering
				entry.nameString = entry.path.filename().string();
				entry.sizeString = entry.isDirectory ? "    ---" : entry.readableSize();
				entry.updateString = entry.readableDate(timeFormat);

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
//	Listing::setSort
//

void Listing::setSort(size_t column, bool ascending) {
	sortColumn = column;
	sortAscending = ascending;
	sort();
}


//
//	Listing::setExtensionFilter
//

void Listing::setExtensionFilter(const std::string& filter) {
	extensions.clear();
	std::stringstream ss(filter);
	std::string extension;

	while (std::getline(ss, extension, ',')) {
		extensions.emplace_back(extension);
	}
}


//
//	Listing::setSearch
//

void Listing::setSearch(const std::string& filter) {
	if (filter.size()) {
		search.assign(filter, std::regex_constants::icase);

	} else {
		search = std::regex();
	}
}


//
//	Listing::forEach
//

void Listing::forEach(std::function<void(const Entry&)> callback) {
	for (auto& entry : *this) {
		if (filter(entry)) {
			callback(entry);
		}
	}
}


//
//	Listing::sort
//

void Listing::sort() {
	// sort current nodes
	std::sort(begin(), end(), [this](const Entry& left, const Entry& right) {
		if (sortColumn == 0) {
			return (sortAscending)
				? left.sortString < right.sortString
				: left.sortString > right.sortString;

		} else if (sortColumn == 1) {
			return (sortAscending)
				? left.lastUpdate < right.lastUpdate
				: left.lastUpdate > right.lastUpdate;

		} else if (sortColumn == 2) {
			return (sortAscending)
				? left.size < right.size
				: left.size > right.size;
		}

		return false;
	});
}


//
//	Listing::filter
//

bool Listing::filter(const Entry& entry) {
	// filter by visibility
	if (!showHidden) {
#ifdef _WIN32
		auto dwAttr = GetFileAttributesW(entry.path.c_str());

		if (dwAttr == INVALID_FILE_ATTRIBUTES) ((dwAttr & FILE_ATTRIBUTE_HIDDEN) != 0) {
			return false;
		}

#else
		auto name = entry.path.filename().string();

		if (name[0] == '.' && name != "." && name != "..") {
			return false;
		}
#endif
	}

	// filter by extension
	if (extensions.size()) {
		if (std::find(extensions.begin(), extensions.end(), entry.extension) != extensions.end()) {
			return false;
		}
	}

	// filter by search
	if (search.mark_count() != 0 && !std::regex_search(entry.nameString, search)) {
		return false;
	}

	return true;
}


//
//	Entry::readableSize
//

std::string Entry::readableSize() {
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
//	Entry::readableDate
//

std::string Entry::readableDate(const std::string& format) {
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
