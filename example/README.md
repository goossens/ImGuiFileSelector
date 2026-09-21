# ImGuiFileSelector Example

This directory contains a full example on how to use the ImGuiFileSelector widget.
It implements a test program to try out various configuration settings and modes.

![Screenshot](screenshot.png)

It uses the SDL3 GPU rendering backend and comes with CMake support so
you should be able to run it on MacOS, Linux and Windows. Here are
the instructions on how to get started based on platform and
development tools.

Recently an optional DirectX version 11 backend has been added to facilitate
testing. To use the DirectX backend instead of the SDL3 GPU backend,
define D3D11 in your build environment before compiling.

## Visual Studio Code

- This should work on MacOS (tested), Linux (untested) and Windows (untested).
- Ensure you have the C++ and CMake extensions activated in your installation.
- Open the top-level ImGuiFileSelector folder.
- As the top-level does not contain a CMakeList.txt file, tell Visual Studio Code you want to use the example/CMakeList.txt file.
- When you build the executable, the dependencies (SDL3, and Dear ImGui) will be automatically downloaded.

## Xcode

- You can generate the Xcode project file from the command line:
	- Install **homebrew** using the instructions at [https://brew.sh](https://brew.sh).
	- Install git using **brew install git** from the command line.
	- Install Cmake using **brew install cmake** from the command line.
	- Run the following in the **example** folder: **cmake -B ../build -G Xcode**.
	- You now have an Xcode project in the top-level **build** folder.
	- This step is only required once.

## Visual Studio

- Visual Studio has CMake integration so you should be able to do it in a similar way to Visual Studio Code.

## Command line

- MacOS:
	- Installing **homebrew** using the instructions at [https://brew.sh](https://brew.sh).
	- Installing git using **brew install git** from the command line.
	- Installing Cmake using **brew install cmake** from the command line.
	- Install Ninja using **brew install ninja** from the command line.

- Linux:
	- Ensure the **build-essential**, **cmake** and **ninja-build** packages are installed.

- All platforms
	- Run the following commands:
		- **cd example**
		- **cmake -B ../build -G "Ninja Multi-Config"**
		- **cmake --build ../build --config Debug**
	- The executable should now be at **ImGuiFileSelector/build/Debug/example**

## Credits

The example application used the following third party open source packages
(which are automatically downloaded by the build process):

* [SDL3](https://github.com/libsdl-org/SDL) - cross-platform library designed to make it easy to write multi-media software released under the [zlib license](https://opensource.org/licenses/Zlib).
* [Dear ImGui](https://github.com/ocornut/imgui) - graphical user interface library released under the [MIT license](https://opensource.org/licenses/MIT).

The example application also uses the following font:

* [DejaVu Font](https://en.wikipedia.org/wiki/DejaVu_fonts) is extracted from
[here](https://github.com/dejavu-fonts/dejavu-fonts) and it was released under a
[generous license](https://github.com/dejavu-fonts/dejavu-fonts/blob/master/LICENSE).
