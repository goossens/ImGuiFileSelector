## API Overview

To use this widget, you need to fullfil 4 requirements:

- Determine how you instantiate the widget.
- Open the widget in one of the supported modes.
- Render the widget as part of your ImGui loop.
- Respond to user selections or cancellations.

### Widget Instantiation
You have two choices for widget instantiation. You can either instantiate it somewhere
in your app (you can even have multiple instances) or you use the included singleton
interface. The FileSelector class maintains state between activations, so if you have
multiple instances, they would potentially have different states. For most applications
using the singleton interface or just have one private instance would ensure that the
state in consistent throughout the app.

``` cpp
	// use the singleton
	auto& selector = FileSelector::Instance();

	// or

	// instantiate a FileSelector yourself
	FileSelector selector.
```

### Open the Widget

To open the widget, you must call the activation function associated with the
intended mode:

- **OpenFile** to popup a selector to open a single file.
- **SaveAs** to popup a selector to save to a single file.
- **SelectFiles** to popup a selector to select one or more files.
- **SelectDirectory** to popup a selector to select a directory.

As long as a selector dialog is open, subsequent calls to any of these functions
will be ignored.

### Rendering the Widget

Somewhere in your Dear ImGui rendering loop you must call the **Render** function.
You can call it every frame as the function does nothing if a file selector
isn't active.

``` cpp
	selector.Render();
```

### Responding to User Interactions

The **Render** function return **true** if the user made a selection or hit cancel.
The following pattern can be used to action the user's request. In case you
don't use all the modes, you obviously only have to handle the ones used.

``` cpp
	if (selector.Render()) {
		if (selector.SelectedOpenFile()) {
			// handle file open
			auto& file = selector.GetSelectedPath();

		} else if (selector.SelectedSaveAs()) {
			// handle select as
			auto& file = selector.GetSelectedPath();

		} else if (selector.SelectedFiles()) {
			// handle select files
			auto& files = selector.GetSelectedPaths();

		} else if (selector.SelectedDirectory()) {
			// handle select directory
			auto directory = selector.GetSelectedPath()

		} else if (selector.WasCancelled()) {
			// handle cancel
		}
	}
```
