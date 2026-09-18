#	Toastr - A selector system for Dear ImGui.
#	Copyright (c) 2026 Johan A. Goossens. All rights reserved.
#
#	This work is licensed under the terms of the MIT license.
#	For a copy, see <https://opensource.org/licenses/MIT>.

include(FetchContent)

if(NOT D3D11)
	add_subdirectory(sdl)
endif()

add_subdirectory(freetype)
add_subdirectory(imgui)
