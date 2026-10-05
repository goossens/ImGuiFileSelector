## Preserving State

The file selector widget maintains state while the application is running
but does not maintain state between application sessions. APIs are however
available to get and set the state which allows applications to save them
wherever they want in whatever format.

``` cpp
	struct State {
		std::filesystem::path currentPath;
		std::vector<std::filesystem::path> recentPlaces;
		bool showSideBar = true;
		bool showHidden = false;
		SortColumn sortColumn = SortColumn::name;
		SortOrder sortOrder = SortOrder::ascending;
		size_t sizeBarWidthInGlyphs = 25;
	};

	const State& GetCurrentState();
	void RestoreState(const State& newState);
```
