#	FileSelector - A file select dialog for Dear ImGui.
#	Copyright (c) 2026 Johan A. Goossens. All rights reserved.
#
#	This work is licensed under the terms of the MIT license.
#	For a copy, see <https://opensource.org/licenses/MIT>.

cmake_minimum_required(VERSION 3.17 FATAL_ERROR)
project(FileSelector VERSION 1.0 DESCRIPTION "FileSelector Example Program" LANGUAGES C CXX)

option(D3D11 "Use DirectX 11 as the backend instead of SDL3 GPU")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory(deps)

file(GLOB SOURCES CONFIGURE_DEPENDS *.cpp ../*.cpp ../*.h)
add_executable(example ${SOURCES})

if(D3D11)
	target_compile_definitions(example PRIVATE D3D11)
	target_link_libraries(example imgui)
else()
	target_link_libraries(example SDL3::SDL3 SDL3_Headers imgui)
endif()

target_compile_options(example PRIVATE
	$<$<COMPILE_LANG_AND_ID:CXX,AppleClang,Clang,GNU>:-Werror -Wall -Wpedantic -Wextra>
	$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/W4 /WX>)

if(CMAKE_GENERATOR STREQUAL "Xcode")
	set_target_properties(example PROPERTIES XCODE_GENERATE_SCHEME ON)
	set_target_properties(example PROPERTIES XCODE_ATTRIBUTE_EXECUTABLE_NAME "example")
endif()
