<div align="center">

![MacOS status](https://img.shields.io/github/actions/workflow/status/goossens/ImGuiFileSelector/macos.yml?branch=main&label=MacOS&style=for-the-badge)
![Linux status](https://img.shields.io/github/actions/workflow/status/goossens/ImGuiFileSelector/linux.yml?branch=main&label=Linux&style=for-the-badge)
![Windows status](https://img.shields.io/github/actions/workflow/status/goossens/ImGuiFileSelector/windows.yml?branch=main&label=Windows&style=for-the-badge)
<br/>
![Repo size](https://img.shields.io/github/repo-size/goossens/ImGuiFileSelector?style=for-the-badge)
![Repo activity](https://img.shields.io/github/commit-activity/m/goossens/ImGuiFileSelector?label=Commits&style=for-the-badge)
<br/>
[![License](https://img.shields.io/badge/License-MIT-yellow.svg?style=for-the-badge)](https://opensource.org/licenses/MIT)
![Maintained](https://img.shields.io/maintenance/yes/2026?style=for-the-badge)

# ImGuiFileSelector

</div>

Simple File Selector for Dear ImGui with a MacOS pedigree/layout and a Dear ImGui look and feel.

![Screenshot](docs/screenshot.png)

## Features

- Works on MacOS, Linux and Windows.
- Works with latest Dear ImGui version (currently v1.92.8 && v1.92.9) and does not use deprecated functions.
- Is C++17 based (not unreasonable in 2026 I think) although Dear ImGui still uses C++11.
- Has no runtime dependencies other than Dear ImGui and the C++17 Standard Template Library (STL).
- Offers four different modes:
	- Select a file to open.
	- Select a file to save to.
	- Select multiple files.
	- select a directory.
- API calls are available to see what user selected, see [example](example/selector.cpp) and [documentation](docs/overview.md).
- Provides optional sidebar for quick navigation to favorites, cloud, locations and/or media.
- Sidebar groups are collapsible and are are hidden when empty.
- Sidebar size can be adjusted by dragging right side.
- Default sidebar is MacOS-like but can be completely [customized/adjusted](docs/sidebar.md).
- Provides directory history navigation (backwards, forwards) as if they are hyperlinks in a browser.
- Directory filtering:
	- Show/hide hidden files/directories.
	- Filter files by extension.
	- Text based name filtering.
- Directories can be sorted (ascending/descending) by name, date or size.
- Quick navigation to any parent directory.
- Quick navigation to places recently visited.
- Ability to create a new folder.
- Context menu for each directory entry to:
	- Rename file or folder.
	- Move file or folder to trash.
	- Duplicate a file or folder.
	- Change file/folder permissions.
- State is automatically preserved between selector popups while application is running.
- API calls are available to [preserve state](docs/state.md) between application runs.

## Versioning

This repository includes releases with a numbering scheme synchronized with Dear ImGui.
This will allow people to quickly find a version of the widgets compatible with a specific Dear ImGui version.

## Issues

If you are interested in using this File Selector, steal parts of the code,
make suggestions for improvements or contribute fixes/enhancements, be my guest as this
repository is released under the MIT license. For people that want to contribute,
[Contributing Guidelines](CONTRIBUTING) and a [Code of Conduct](CODE_OF_CONDUCT.md)
are available. If you find any problems or want to make a suggestion for improvement, please
[raise an issue on this repository](https://github.com/goossens/ImGuiFileSelector/issues).

## Credits

ImGuiFileSelector was written from scratch by [Johan A. Goossens](https://github.com/goossens)
and if you end up using (parts of) this repository, a shoutout or Github star would be appreciated.
Other notification systems exist on Git but many of them have not been maintained for a bit.

A big thank you to [Omar Cornut](https://github.com/ocornut/imgui) for creating Dear ImGui
in the first place. Without you, this notification system would not exist.

## License

This work is licensed under the terms of the MIT license.
For a copy, see <https://opensource.org/licenses/MIT>.
