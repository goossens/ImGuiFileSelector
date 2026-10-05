## File Selector Side Bar

This widget offers a shortcut side bar on the left of the selector. The layout
of the side bar is modelled after MacOS but the entries are different for the
various operating systems. Upon instantiation, the side bar is configured with a
default which can be overridden or enhanced by the application using the following
API calls on the FileSelector instance.

``` cpp
	void ClearFavorites();
	void AddFavorite(const std::string& name, const std::filesystem::path& path);
	void AddDefaultFavorites();

	void ClearClouds();
	void AddCloud(const std::string& name, const std::filesystem::path& path);
	void AddDefaultClouds();

	void ClearLocations();
	void AddLocation(const std::string& name, const std::filesystem::path& path);
	void AddDefaultLocations();

	void ClearMedia() { media.entries.clear(); }
	void AddMedia(const std::string& name, const std::filesystem::path& path);
	void AddDefaultMedia() { addDefaultMedia(); }
```

If any of the 4 sections are empty, they will be removed from view.
