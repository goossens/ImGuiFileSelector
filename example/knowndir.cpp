//	FileSelector - A file select dialog for Dear ImGui.
//	Copyright (c) 2026 Johan A. Goossens. All rights reserved.
//
//	This work is licensed under the terms of the MIT license.
//	For a copy, see <https://opensource.org/licenses/MIT>.


//
//	Include files
//

#include <cstdlib>

#include "knowndir.h"


#if __APPLE__


//
//	getKnownDirectoryInfo (MacOS implementation)
//

#include <vector>
#include <CoreFoundation/CoreFoundation.h>

void getKnownDirectoryInfo(KnownDirectory type, std::string& label, std::filesystem::path& path) {
	// set default values
	static const char* types[] = {
		"Desktop",
		"Documents",
		"Downloads",
		"Movies",
		"Music",
		"Pictures"
	};

	label = types[static_cast<size_t>(type)];
	std::filesystem::path home(std::getenv("HOME"));
	path = home / label;

	// use CoreFoundation to find localized display name
	CFStringRef cfPath = CFStringCreateWithCString(kCFAllocatorDefault, path.c_str(), kCFStringEncodingUTF8);
	CFURLRef cfURL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, cfPath, kCFURLPOSIXPathStyle, true);
	CFStringRef localizedNameRef = nullptr;

	if (CFURLCopyResourcePropertyForKey(cfURL, kCFURLLocalizedNameKey, &localizedNameRef, nullptr)) {
		CFIndex length = CFStringGetLength(localizedNameRef);
		CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;

		std::vector<char> buffer(maxSize);
		CFStringGetCString(localizedNameRef, buffer.data(), maxSize, kCFStringEncodingUTF8);
		label = std::string(buffer.data());

		CFRelease(localizedNameRef);
	}

	CFRelease(cfURL);
	CFRelease(cfPath);
}


#elif _WIN32


//
//	getKnownDirectoryInfo (Windows implementation)
//

#include <windows.h>
#include <shlobj.h>

void getKnownDirectoryInfo(KnownDirectory type, std::string& label, std::filesystem::path& path) {
	// set default values
	static const char* types[] = {
		"Desktop",
		"Documents",
		"Downloads",
		"Videos",
		"Music",
		"Pictures"
	};

	label = types[static_cast<size_t>(type)];
	std::filesystem::path home(std::getenv("USERPROFILE"));
	path = home / label;

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
	PWSTR path = NULL;

	if (SUCCEEDED(SHGetKnownFolderPath(IDs[type], 0, NULL, &path))) {
		path = path;
		CoTaskMemFree(path);
	}

	// uninitialize COM library
	CoUninitialize();
}


#else


//
//	getKnownDirectoryInfo (Linux/BSD implementation)
//

#include <fstream>

void getKnownDirectoryInfo(KnownDirectory type, std::string& label, std::filesystem::path& path) {
	// set default values
	static const char* types[] = {
		"Desktop",
		"Documents",
		"Downloads",
		"Videos",
		"Music",
		"Pictures"
	};

	label = types[static_cast<size_t>(type)];
	std::filesystem::path home(std::getenv("USERPROFILE"));
	path = home / label;

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
					path = (rawPath.rfind("$HOME", 0) == 0) ? home / rawPath.substr(5) : rawPath;
					label = path.filename().string();
				}
			}
		}
	}
}


#endif
