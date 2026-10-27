## Custom Icons

By default, the file selector uses a small set of built-in icons for folders and specific
file types (based on file extension). An application can however provide a callback that
will render custom icons.

When setup, the custom renderer is called for each icon and is provided with relevant data
to do that rendering and upon completion, it can return true or false. When false is returned,
the file selector will still use the built-in icons for that object.

The provided rendering data looks like:

``` c++
	// data for a custom icon renderer
	struct IconData {
		// top left corner of icon
		ImVec2 pos;

		// scale of icon
		// use this if icons are font based (e.g. ImGui::PushFont(iconFont, scale * ImGui::GetStyle().FontSizeBase))
		float scale;

		// size of icon in pixels = GetFontSize() * scale
		ImVec2 size;

		// drawlist to render to
		ImDrawList* drawList;

		// item information
		bool isDirectory;
		std::string extension;
	};
```

As an example, here is a custom icon rendering callback that draws a circle for folders
and lets the file selector draw the file icons.

``` c++
	selector.SetCustomIconRenderer([](const FileSelector::IconData& data) {
		if (data.isDirectory) {
			const auto center = data.pos + data.size * 0.5f;
			const auto radius = data.size.x * 0.5f;
			const auto color = ImGui::GetColorU32(ImGuiCol_TextDisabled);
			data.drawList->AddCircleFilled(center, radius, color);
			return true;

		} else {
			return false;
		}
	});
```
