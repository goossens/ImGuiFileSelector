//	FileSelector - A file select dialog for Dear ImGui.
//	Copyright (c) 2026 Johan A. Goossens. All rights reserved.
//
//	This work is licensed under the terms of the MIT license.
//	For a copy, see <https://opensource.org/licenses/MIT>.


//
//	Include files
//

#include "trashcan.h"


#if __APPLE__

//
//	movePathToTrashCan (MacOS implementation)
//

#include <CoreFoundation/CoreFoundation.h>
#include <objc/runtime.h>
#include <objc/message.h>

extern "C" {
    void *objc_autoreleasePoolPush(void);
    void objc_autoreleasePoolPop(void* pool);
}

bool movePathToTrashCan(const std::filesystem::path& path) {
	// determine absolute path with .. and symbolic links resolved
	auto canonicalPath = std::filesystem::canonical(path);
	auto canonicalString = canonicalPath.string();

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
}


#elif _WIN32

//
//	movePathToTrashCan (Windowz implementation)
//

#include <windows.h>
#include <shellapi.h>

bool movePathToTrashCan(const std::filesystem::path& path) {
	// determine absolute path with .. and symbolic links resolved
	auto canonicalPath = std::filesystem::canonical(path);
	auto canonicalString = canonicalPath.wstring();

	// double terminate string for WIN32 API
	canonicalString.push_back(L'\0');
	canonicalString.push_back(L'\0');

	// run native API
	SHFILEOPSTRUCTW fileOp = {0};
	fileOp.wFunc = FO_DELETE;
	fileOp.pFrom = canonicalString.data();
	fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;
	return SHFileOperationW(&fileOp) == 0;
}


#else

//
//	movePathToTrashCan (Linux/BSD implementation)
//

#include <cstdlib>

bool movePathToTrashCan(const std::filesystem::path& path) {
	// determine absolute path with .. and symbolic links resolved
	auto canonicalPath = std::filesystem::canonical(path);
	auto canonicalString = canonicalPath.string();

	// use desktop command to move files to trash
	std::string command = "gio trash '" + canonicalString.string() + "'";
	return std::system(command.c_str()) == 0;
}

#endif
